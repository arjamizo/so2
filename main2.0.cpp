#include <ncurses.h>
#include <algorithm>
#include <unistd.h>
#include <pthread.h>
#include <stdexcept>
#include <cassert>
#include <queue>
#include <cstring>
#include <cmath>
using namespace std;

bool paused=false,step=false;

int waterHeight=12;
int roadWidth=6;
int kolumny, rzedy;
int bridgeOnWaterXLeft=kolumny/2-roadWidth/2;
int bridgeOnWaterXRight=kolumny/2+roadWidth/2-1;
int bridgeOnWaterTop=rzedy/2-waterHeight/2;
int bridgeOnWaterYBottom=rzedy/2+waterHeight/2;

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

typedef pthread_mutex_t Mutex;
enum Type {CLASS_SHIP, CLASS_CAR_NtoS, CLASS_CAR_StoN};
struct Animable;
typedef int (*fptr)(const Animable *context, Animable* &waitingFor, int destX, int destY);

int countObjectsOnBridge();
int isShipOnRight(Animable*& waitingFor);
struct Animable
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
    bool sleeping;
    Type type;
    pthread_cond_t cond;
    Mutex mutex;
    void runit()
    {
        pthreadpool.pthread_create(&tid, (const pthread_attr_t*)NULL, threadFunctionAccessibleFromOutsideWrapper, (void *)this);
        fprintf(stderr, "ran thread %s\n", str);
    }
    static fptr canMoveCallback;
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
        sleeping=true;
    };

    //checks whether (x,y) is in area which object is painted in
    bool takesField(int x, int y)
    {
        return (posx+dx<=x && x<=posx+width-dx && posy-dy<=y && y<posy+height+dy);
    }
    bool isInArea(int x, int y, int ty, int by, int lx, int rx)
    {
        if (x==-1 and y==-1) x=posx,y=posy;
        return (lx+dx<=x && x<rx-dx && ty+dy<y && y<by-dy);
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
        bool sleeping=this->sleeping;
        if(sleeping) attron(COLOR_PAIR(4));//woda
        if(shouldBeDrawn)
        {
            for(int i=0; i<width; ++i)
                for(int j=0; j<height; ++j)
                {
                    mvprintw(posy+j, posx+i, str);
                }
        }
        if(sleeping) attroff(COLOR_PAIR(4));
    }

    Animable* setCanMoveSpecifiedCallback(fptr callback)
    {
        //canMoveCallback=callback;
        assert(0);
        return this;
    }
    static void setCanMoveCallback(fptr callback)
    {
        canMoveCallback=callback;
        return;
    }

    int canMove(Animable **waitingFor=0)
    {
        if(waitingFor) *waitingFor=NULL;
        int destX, destY;
        destX=fx+(tx-fx)*curTime/totalTime;
        destY=fy+(ty-fy)*curTime/totalTime;
        if (canMoveCallback!=NULL)
        {
            Animable *collision;
            if(canMoveCallback(this, collision, destX, destY)==false)
            {
                fprintf(stderr, "canMove(): %s is waiting for %s\n", this->str, collision->str);
                if(waitingFor) *waitingFor=collision;
                return this->type==CLASS_SHIP;
            }
        }
        else
        {
            perror("No canMove callback handler");
            exit(1);
        }
        return true;
    }

    int steps;
    //-1 - animation finished, thread deleted
    //0 - waiting for signal from monitor
    //1 - handle animatoin in next step
    virtual int tick()
    {

        int destX, destY;
        destX=fx+(tx-fx)*curTime/totalTime;
        destY=fy+(ty-fy)*curTime/totalTime;
        Animable *waitingFor;
        fprintf(stderr, "%s ticking, %d. destx %d desty %d, isShipOnRight=%d countObjectsOnBridge=%d\n", str, steps++, destX, destY, (int)isShipOnRight(waitingFor), countObjectsOnBridge());
        float px=float(posx-fx)/float(tx-fx), py=float(posy-fy)/float(ty-fy);
        if(curTime>totalTime)
        {
            fprintf(stderr, "%s finished.\n", str);
            fflush(stderr);
            shouldBeDrawn=false;
            return -1;
        }

        curTime++;
        posx=destX;
        posy=destY;
        shouldBeDrawn=true;
        pthread_cond_broadcast(&cond);
        fprintf(stderr, "progress x=%f py=%f fx=%d fy=%d posx=%d posy=%d tx=%d ty=%d cT=%d tlTime=%d t=%s\n", px, py, fx, fy
                , posx, posy
                , tx, ty
                , curTime, totalTime, str);
        fflush(stderr);
        return 1;
    }
    virtual void *thread()
    {
        fprintf(stderr, "thread started\n");
        int ret=0;
        do
        {
            //fprintf(stderr, "tickig %s\n", this->str);
            Animable *waitingFor;
            if(!this->canMove(&waitingFor))
            {
                fprintf(stderr, "%s thread is waiting for signal from %s\n", str, waitingFor->str);
                MutexLock lock(waitingFor->mutex);
                this->sleeping=true;
                while(!this->canMove(&waitingFor)
                        || (waitingFor && waitingFor->type==CLASS_SHIP && waitingFor->posx > bridgeOnWaterXLeft))
                {
                    fprintf(stderr, "%s thread is waiting for signal from %s in while loop\n", str, waitingFor->str);
                    pthread_cond_wait(&waitingFor->cond, &waitingFor->mutex);
                    fprintf(stderr, "%s received signal from %s\n", str, waitingFor->str);
                    fflush(stderr);
                }
                this->sleeping=false;
            }
            //do usleep(1000*1000/this->speed); while (paused || !step);step=false;
            (ret=this->tick())==0;
            //pthread_yield();
            if(ret==1)
            {
                usleep(1000*1000/this->speed);
            }
        }
        while (ret!=-1);
        pthread_cond_broadcast(&cond);
        fprintf(stderr, "%s sent conditional variable broadcast\n", str);
        fflush(stderr);
        pthreadpool.remove(pthread_self());
        return 0;
    }
};

fptr Animable::canMoveCallback;
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
        Animable(CLASS_SHIP, label, kolumny-1, rzedy/2, 0, rzedy/2,20,4)
    {
        fprintf(stderr, "created ship %d %d %d %d\n", kolumny-1, rzedy/2, 0, rzedy/2);
    }
    Ship(char c, int speed=5):
        Animable(CLASS_SHIP, NULL, kolumny-1, rzedy/2, 0, rzedy/2,20,4)
    {
        snprintf(str, sizeof str, "%c", c);
    };
};

void *animateThread(void *obj)
{
    Animable* This=(Animable*)obj;
    fprintf(stderr, "trying to run %s\n", ((Animable*)This)->str);
    This->sleeping=false;
    return (This->thread());
}

void runThread(void *arg)
{
    pthread_t tid;
    pthreadpool.pthread_create(&tid, (const pthread_attr_t*)NULL, animateThread, (void *)arg);
}

void displayLastError()
{
    printf("cz\n");
    perror("blad? ");
    fclose(stderr);
}
vector<Animable*> dynamically_created;
Mutex dynamically_created_mutex=PTHREAD_MUTEX_INITIALIZER;

void *drawingThread(void *)
{
    rzedy=20, kolumny=30;
    while (1)
    {
        MutexLock lock(dynamically_created_mutex);
        //MutexLock lock(ncursesMutex);
        //pthread_barrier_init(&synchro.barrier,NULL,synchro.cntr);
        //pthread_cond_broadcast(&cond_barrier_initialized);
        getmaxyx(stdscr, rzedy, kolumny); //1
        erase();
        attron(COLOR_PAIR(1));//trawa
        for(int y=0; y<rzedy; ++y) for(int x=0; x<kolumny; ++x) mvprintw(y,x," ");
        attroff(COLOR_PAIR(1));
        attron(COLOR_PAIR(2));//woda
        for(int y=rzedy/2-waterHeight/2; y<rzedy/2+waterHeight/2; ++y) for(int x=0; x<kolumny; ++x) mvprintw(y,x,"~");
        attroff(COLOR_PAIR(2));

        for(int i=0; i<dynamically_created.size(); ++i)
            if (dynamically_created[i]->type==CLASS_SHIP)
            {
                dynamically_created[i]->draw();
            }
        bridgeOnWaterXLeft=kolumny/2-roadWidth/2;
        bridgeOnWaterXRight=kolumny/2+roadWidth/2-1;
        bridgeOnWaterTop=rzedy/2-waterHeight/2-1;
        bridgeOnWaterYBottom=rzedy/2+waterHeight/2-1;
        attron(COLOR_PAIR(3));//asfalt
        for(int y=0; y<rzedy; ++y) for(int x=kolumny/2-roadWidth/2; x<kolumny/2+roadWidth/2; ++x) mvprintw(y,x,"~");
        attroff(COLOR_PAIR(3));

        for(int i=0; i<dynamically_created.size(); ++i)
            if (dynamically_created[i]->type!=CLASS_SHIP)
            {
                //dynamically_created[i]->tick();
                dynamically_created[i]->draw();
            }

        attron(COLOR_PAIR(1));//trawa
        move(rzedy-1,kolumny-1);
        attroff(COLOR_PAIR(1));//trawa
        refresh();
    }
    return 0;
}

int isShipOnRight(Animable*& waitingFor)
{
    waitingFor=NULL;
    for(int i=0; i<dynamically_created.size(); ++i)
    {
        if(dynamically_created[i]->type==CLASS_SHIP and dynamically_created[i]->posx+dynamically_created[i]->width >= bridgeOnWaterXRight)
        {
            fprintf(stderr, "found ship %s having posx=%d >= %d\n", dynamically_created[i]->str
                    , dynamically_created[i]->posx
                    , bridgeOnWaterXRight);
            fflush(stderr);
            waitingFor=dynamically_created[i];
            return true;
        }
    }
    return false;
}

int countObjectsOnBridge()
{
    int cnt=0;
    for(int i=0; i<dynamically_created.size(); ++i)
    {
        if (dynamically_created[i]->type!=CLASS_SHIP
                && dynamically_created[i]->isInArea(-1,-1,bridgeOnWaterTop,bridgeOnWaterYBottom,bridgeOnWaterXLeft,bridgeOnWaterXRight))
            cnt++;
    }
    return cnt;
}

bool canEnterBridge(const Animable* context, Animable*& mustWaitFor)
{
    if(countObjectsOnBridge()>=3) return false;
    for(int i=0; i<dynamically_created.size(); ++i)
    {
        if (dynamically_created[i]->type!=CLASS_SHIP
                && dynamically_created[i]->isInArea(-1,-1,bridgeOnWaterTop,bridgeOnWaterYBottom,bridgeOnWaterXLeft,bridgeOnWaterXRight))
            return context->dy==dynamically_created[i]->dy;
    }
}

//-1 - animation finished, thread deleted
//0 - waiting for signal from monitor
//1 - handle animatoin in next step
int canMoveCallback(const Animable* context, Animable*& waitingFor, int destX, int destY)
{
    for(int i=0; i<dynamically_created.size(); ++i)
    {
        Animable* obj=dynamically_created[i];
        if(obj==context || !obj->shouldBeDrawn) continue;
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
        if(((destY==bridgeOnWaterTop && context->dy<0) or (destY==bridgeOnWaterYBottom && context->dy>0))
                && isShipOnRight(waitingFor))
        {
            waitingFor=obj;
            fprintf(stderr, "%s stopped because ship %s on right part of screen, %d==%d or %d==%d\n", context->str, waitingFor->str
                    , destY, bridgeOnWaterTop, destY, bridgeOnWaterYBottom
                   );
            waitingFor=obj;
            return 0;
        }
        if(!canEnterBridge(context, waitingFor) &&
                ((abs(destY-bridgeOnWaterTop)<=1) && context->dy<0) or (abs(destY-bridgeOnWaterYBottom+context->height-1)<=1 && context->dy>0))
        {
            waitingFor=obj;
            fprintf(stderr, "%s stopped because there are 3 or more cars on bridge, %d==%d or %d==%d\n", context->str, waitingFor->str
                    , destY, bridgeOnWaterTop, destY, bridgeOnWaterYBottom
                   );
            waitingFor=obj;
            return 0;
        }
    }
    return 1;
}

vector<Animable*> objects;
int main(int argc, char *argv[])
{
    atexit(displayLastError);
    initscr();
    start_color();
    getmaxyx(stdscr, rzedy, kolumny);
    stderr=fopen("output.txt", "w+");
    fprintf(stderr, "wczytano %d %d\n", rzedy, kolumny);
    init_pair(1, COLOR_CYAN, COLOR_GREEN); //trawa
    init_pair(2, COLOR_BLUE, COLOR_BLUE); //woda
    init_pair(3, COLOR_WHITE, COLOR_WHITE); //asfalt
    init_pair(4, COLOR_WHITE, COLOR_YELLOW); //uspiony
    pthread_t tid;
    pthread_mutex_init(&dynamically_created_mutex, NULL);
    pthreadpool.pthread_create(&tid, NULL, drawingThread, (void *)NULL);
    Animable::setCanMoveCallback(canMoveCallback); //this function is called eveytime any object wants to move
    {
        sleep(1);
        MutexLock lock(dynamically_created_mutex);
        dynamically_created.push_back((new CarNS("B"))->setSpeed(6));
        runThread(dynamically_created.back());
        dynamically_created.push_back((new Ship("A"))->setSpeed(20));
        runThread(dynamically_created.back());
        dynamically_created.push_back((new CarSN("C"))->setSpeed(6));
        runThread(dynamically_created.back());
    }
    int i=4;
    char name[100];
    name[0]='A';
    name[1]=0;
    while(1)
    {
        char k;
        while((k=getch())!='r')
        {
            name[0]='A'+i;
            MutexLock lock(dynamically_created_mutex); //prawdopodobnie lista obiektow sie zmieni, wiec trzeba miec do niej wylacznosc
            switch(k)
            {
            case 's':
                dynamically_created.push_back((new Ship(name))->setSpeed(15));
                runThread(dynamically_created.back());
                i++;
                break;
            case 'd':
                dynamically_created.push_back(new CarNS(name));
                runThread(dynamically_created.back());
                i++;
                break;
            case 'c':
                dynamically_created.push_back(new CarSN(name));
                runThread(dynamically_created.back());
                i++;
                break;
            case 'p':
                paused=!paused;
                fprintf(stderr, paused?"paused\n":"unpaused\n");
                fflush(stderr);
                break;
            case 'n':
                step=true;
                break;
            }
        }
        pthreadpool.removeAll();
    }
}

