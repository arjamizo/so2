#include <ncurses.h>
#include <algorithm>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include <stdexcept>
#include <cassert>
#include <queue>
#include <cstring>
using namespace std;
//zmienne
int kolumny = 0;
int rzedy = 0;
char tekst[] = "Wyswietlany tekst!";

typedef pthread_mutex_t Mutex;

class MutexLock   //Resource acquisition is initialization!
{
public:
    MutexLock(pthread_mutex_t& mutex)
        : m_mutex(mutex)
    {
        if (pthread_mutex_lock(&m_mutex))
            throw std::runtime_error("Could not lock the mutex.");
    }
    ~MutexLock()
    {
        pthread_mutex_unlock(&m_mutex);
    }
private:
    pthread_mutex_t& m_mutex;
};


struct ThreadsPool
{
#define MAX_THREADS 2000
    pthread_t threads[MAX_THREADS];
    void remove(pthread_t id)
    {
        for(int i=0; i<MAX_THREADS; ++i)
            if(threads[i]==id)
            {
                pthread_cancel(threads[i]);
                pthread_join(threads[i], NULL);
                threads[i]=0;
                return;
            }
        perror("This should never happen. ThreadsPool::remove error.");
        exit(1);
    }
    void removeAll()
    {
        for(int i=0; i<MAX_THREADS; ++i)
            if(threads[i]!=0)
            {
                pthread_cancel(threads[i]);
                pthread_join(threads[i], NULL); //http://jedrzej.ulasiewicz.staff.iiar.pwr.wroc.pl/ProgramowanieWspolbiezne/lab/
                //TODO pthread_join
                threads[i]=0;
            }
    }
    pthread_t pthread_create (pthread_t *__restrict __newthread,
                              __const pthread_attr_t *__restrict __attr,
                              void *(*__start_routine) (void *),
                              void *__restrict __arg)
    {
        int rc=::pthread_create(__newthread, __attr, __start_routine, (void *)__arg);
        if (rc)
        {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
        else
            for(int i=0; i<MAX_THREADS; ++i)
                if(threads[i]==0)
                {
                    threads[i]=*__newthread;
                    return rc;
                }
        perror("ThreadsPool::pthread_create error. Probably, we could not find free space for threads in thread pool. TODO.");
        exit(123);
    }
} pthreadpool;

void runThread(void *arg);

enum Type {CLASS_SHIP, CLASS_CAR_NtoS, CLASS_CAR_StoN};
struct Animable;
typedef int (*fptr)(const Animable *context, Animable* &waitingFor, int destX, int destY);
struct Animable: public Drawable
{
    Animable() {};
    char str[100];
    volatile int posx, posy, fx,tx,fy,ty;
    int width, height;
    int curTime,totalTime;
    int speed;
    int dx, dy;
    bool shouldBeDrawn;
    pthread_t tid;
    Type type;
    pthread_cond_t cond;
    Mutex mutex;
    void runit()
    {
        pthreadpool.pthread_create(&tid, (const pthread_attr_t*)NULL, threadFunctionAccessibleFromOutsideWrapper, (void *)this);
        fprintf(stderr, "ran thread %s\n", str);
    }
    Animable *setSpeed(int speed)
    {
        this->speed=speed;
        return this;
    }
    Animable(Type type, const char* c, int fx, int fy, int tx, int ty, int w=1, int h=1):
        type(type),posx(fx),posy(fy),fx(fx),fy(fy),tx(tx),ty(ty),width(w),height(h)
    {
        dx=fx>tx?1:-1;
        dy=fy>ty?1:-1;
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&cond, NULL);
        snprintf(str, sizeof str, "%s", c);
        assert(c);
        fprintf(stderr,"created new %s fx%d tx%d fy%d ty%d w%d h%d\n", str, fx,tx,fy,ty,w,h);
        fflush(stderr);

        curTime=-1;
        shouldBeDrawn=false;
        totalTime=max(abs(tx-fx),abs(ty-fy));
        speed=3;
    };

    //checks whether (x,y) is in area which object is painted in
    bool takesField(int x, int y)
    {
        return (posx+2*dx<=x && x<=posx+width-2*dx && posy+2*dy<=y && y<=posy+height-2*dy);
    }
    void *thread()
    {
        fprintf(stderr, "thread started\n");
        int ret=0;
        do
        {
            //fprintf(stderr, "tickig %s\n", this->str);
            Animable *waitingFor;
            while((ret=this->tick(&waitingFor))==0);
            if(ret==1) usleep(1000*1000/this->speed);
        }
        while (ret!=-1);
        pthreadpool.remove(pthread_self());
        return 0;
    }
    //This is a trick allowing to pass this function as pthread_create_thread parameter, required parameter is pointer to item of this class, because pointer to class method cannot be passed directly to pthread_create_thread
    //http://stackoverflow.com/questions/1151582/pthread-function-from-a-class
    static void *threadFunctionAccessibleFromOutsideWrapper(void *This)
    {
        fprintf(stderr, "trying to run %s\n", ((Animable*)This)->str);
        return (((Animable*)This)->thread());
    }
    void draw(bool selected=false)
    {
        if(shouldBeDrawn)
        {
            for(int i=0; i<width; ++i)
                for(int j=0; j<height; ++j)
                {
                    mvprintw(posy+j, posx+i, str);
                }
        }
    }
    fptr canMoveCallback;

    Animable* setCanMoveCallback(fptr callback)
    {
        canMoveCallback=callback;
        return this;
    }

    int canMove(Animable **waitingFor=0)
    {
        int destX, destY;

        destX=fx+(tx-fx)*curTime/totalTime;
        destY=fy+(ty-fy)*curTime/totalTime;
        if (canMoveCallback!=NULL)
        {
            Animable *collision;
            if(canMoveCallback(this, collision, destX, destY)==false)
            {
                fprintf(stderr, "waiting %s\n", collision->str);
                if(waitingFor) *waitingFor=collision;
                return false;
            }
        }
        return true;
    }

    int steps;
    //-1 - animation finished, thread deleted
    //0 - waiting for signal from monitor
    //1 - handle animatoin in next step
    virtual int tick(Animable **waitingFor=0)
    {

        fprintf(stderr, "%s ticking, %d.\n", str, steps++);
        float px=float(posx-fx)/float(tx-fx), py=float(posy-fy)/float(ty-fy);
        int destX, destY;

        destX=fx+(tx-fx)*curTime/totalTime;
        destY=fy+(ty-fy)*curTime/totalTime;
        if(curTime==totalTime)
        {
            fprintf(stderr, "%s finished.\n", str);
            fflush(stderr);
            shouldBeDrawn=false;
            return -1;
        }
        if (canMoveCallback!=NULL)
        {
            Animable *collision;
            while(canMoveCallback(this, collision, destX, destY)==false)
            {
                fprintf(stderr, "waiting %s\n", collision->str);
                if(waitingFor) *waitingFor=collision;
                return 0;
            }
        }

        curTime++;
        posx=destX;
        posy=destY;
        shouldBeDrawn=true;
        pthread_cond_broadcast(&cond);
        fprintf(stderr, "%s sent conditional variable broadcast\n", str);
        fflush(stderr);
        return 1;
    }
};

int waterHeight=20;
int roadWidth=6;
Mutex ncursesMutex = PTHREAD_MUTEX_INITIALIZER;
struct CarNS: public Animable
{
    CarNS(const char *label, int speed=5):
        Animable(CLASS_CAR_NtoS, label, kolumny/2-roadWidth/2+1, 0, kolumny/2-roadWidth/2+1, rzedy-1,2,2)
    {

    }
    CarNS(char c, int speed=5)
    {
        char label[100];
        snprintf(label,100,"%c",c);
        CarNS(label, speed);
    };
};

struct CarSN: public Animable
{
    CarSN(const char *label, int speed=5):
        Animable(CLASS_CAR_StoN, label, kolumny/2-roadWidth/2+1+2, rzedy-1, kolumny/2-roadWidth/2+1+2, 0,2,2)
    {

    }
    CarSN(char c, int speed=5)
    {
        char label[100];
        snprintf(label,100,"%c",c);
        CarSN(label, speed);
    };
};

struct Ship: public Animable
{
    Ship(const char *label, int speed=5):
        Animable(CLASS_SHIP, "CZE", 25, 25, 0, 0,20,4)
    {
        fprintf(stderr, "created ship %d %d %d %d\n", kolumny-1, rzedy/2, 0, rzedy/2);
    }
};

void *animateThread(void *obj)
{
    Animable* This=(Animable*)obj;
    fprintf(stderr, "trying to run %s\n", ((Animable*)This)->str);
    return (This->thread());
}

void runThread(void *arg)
{
    pthread_t tid;
    pthreadpool.pthread_create(&tid, (const pthread_attr_t*)NULL, animateThread, (void *)arg);
}



Mutex screenmutex;
Mutex animationmutex;

vector<Animable*> dynamically_created;
void *drawingThread(void *)
{
    rzedy=20, kolumny=30;
    //while (1)
    {
        fprintf(stderr, "HERE1\n");
        fprintf(stderr, "HERE2\n");
        erase();
        attron(COLOR_PAIR(1));//trawa
        for(int y=0; y<rzedy; ++y) for(int x=0; x<kolumny; ++x) mvprintw(y,x," ");
        attroff(COLOR_PAIR(1));
        attron(COLOR_PAIR(2));//woda
        for(int y=rzedy/2-waterHeight/2; y<rzedy/2+waterHeight/2; ++y) for(int x=0; x<kolumny; ++x) mvprintw(y,x,"~");
        attroff(COLOR_PAIR(2));


        for(int i=0; i<dynamically_created.size(); ++i)
            if (dynamically_created[i]->type!=CLASS_SHIP)
            {
                dynamically_created[i]->tick();
                dynamically_created[i]->draw();
            }
        attron(COLOR_PAIR(3));//asfalt
        for(int y=0; y<rzedy; ++y) for(int x=kolumny/2-roadWidth/2; x<kolumny/2+roadWidth/2; ++x) mvprintw(y,x,"~");
        attroff(COLOR_PAIR(3));

        for(int i=0; i<dynamically_created.size(); ++i)
            if (dynamically_created[i]->type==CLASS_SHIP)
            {
                dynamically_created[i]->tick();
                dynamically_created[i]->draw();
            }

        attron(COLOR_PAIR(1));//trawa
        move(rzedy-1,kolumny-1);
        attroff(COLOR_PAIR(1));//trawa
        pthread_yield(); //let other threads work
        refresh();
        usleep(100*1000);
    }
    return 0;
}
bool paused,once;

//-1 - animation finished, thread deleted
//0 - waiting for signal from monitor
//1 - handle animatoin in next step
int canMoveCallback(const Animable* context, Animable*& waitingFor, int destX, int destY)
{
    for(int i=0; i<dynamically_created.size(); ++i)
    {
        Animable* obj=dynamically_created[i];
        if(obj==context) continue;
        if(obj->takesField(destX,destY))
        {
            waitingFor=obj;
            fprintf(stderr, "%s stopped because of %s, (%d,%d) is in range (%d..%d, %d..%d)\n", context->str, waitingFor->str
                    , destX, destY
                    , waitingFor->posx, waitingFor->posx+waitingFor->width
                    , waitingFor->posy, waitingFor->posy+waitingFor->height
                   );
            waitingFor=obj;
            return 0;
        }
    }
    return 1;
}

void displayLastError()
{
}

int main(int argc, char *argv[])
{
    atexit(displayLastError);
    initscr();
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_GREEN); //trawa
    init_pair(2, COLOR_BLUE, COLOR_BLUE); //woda
    init_pair(3, COLOR_WHITE, COLOR_WHITE); //asfalt
    stderr=fopen("output.txt", "w+");
    int selected=0;
    getmaxyx(stdscr, rzedy, kolumny); //1
    fprintf(stderr, "Got screen size %d %d\n", rzedy, kolumny);
    pthread_mutex_init(&screenmutex, NULL);
    pthread_mutex_init(&animationmutex, NULL);
    pthread_mutex_init(&ncursesMutex, NULL);
    while(1)
    {
        pthread_t tid;
        getmaxyx(stdscr, rzedy, kolumny); //1
        fprintf(stderr, "Got screen size %d %d\n", rzedy, kolumny);
        synchro.cntr=1;
        int ile=0;
        sleep(1);
        paused=once=false;
        printf("here\n");
        int k;
        char label[10];
        label[0]='A';
        label[1]=0;
        while((k=getch())!='r')
        {
            drawingThread(0);
            label[0]='A'+ile%('z'-'a');
            switch(k)
            {
            case 'd':
            {
                //new CarNS(label))->setSpeed(5)
                //runThread(dynamically_created[ile++]=(new CarNS(label))->setSpeed(5));
                fprintf(stderr, "created new carNS\n");
                fflush(stderr);
            }
            break;
            case 'c':
            {
                runThread(dynamically_created[ile++]=(new CarSN(label))->setSpeed(5));
                fprintf(stderr, "created new car2\n");
                fflush(stderr);
            }
            break;
            case 's':
            {
                //dynamically_created[ile++]=(new Ship(label))->setSpeed(5);
                runThread(dynamically_created[ile++]=(new Ship(label))->setSpeed(5));
                fprintf(stderr, "created new ship\n");
                fflush(stderr);
            }
            break;
            case '[':
                selected--;
                fprintf(stderr, "wybrano %d\n", selected);
                fflush(stderr);
                break;
            case ']':
                selected++;
                fprintf(stderr, "wybrano %d\n", selected);
                fflush(stderr);
                break;
            case '=':
                dynamically_created[selected]->speed+=1;
                break;
            case '-':
                if(dynamically_created[selected]->speed>1)dynamically_created[selected]->speed-=1;
                break;
            }
        };
        for(int i=0; i<dynamically_created.size(); ++i)
        {
            delete dynamically_created[i];
            dynamically_created[i]=0;
        }
        pthreadpool.removeAll();
    }
    // endwin();
    /* Last thing that main() should do */
    pthread_exit(NULL);
}
