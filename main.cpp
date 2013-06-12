#include <ncurses.h>
#include <algorithm>
#include <unistd.h>
#include <pthread.h>
#include <stdexcept>
#include <cassert>
using namespace std;
//zmienne
int kolumny = 0;
int rzedy = 0;
char tekst[] = "Wyswietlany tekst!";

typedef pthread_mutex_t Mutex;

class MutexLock { //Resource acquisition is initialization!
public:
    MutexLock(pthread_mutex_t& mutex)
        : m_mutex(mutex)
    {
        if (pthread_mutex_lock(&m_mutex))
            throw std::runtime_error("Could not lock the mutex.");
    }
    ~MutexLock() { pthread_mutex_unlock(&m_mutex); }
private:
    pthread_mutex_t& m_mutex;
};
int getFloatLen(float a)
{

}

struct Drawable
{
    int w, h;
    int centerto; //0 for center to top left corner
    virtual bool drawtick()
    {
        printf("drawing not implemented\n");
        return true;
    }
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

#include <queue>
struct ConQueue { //facade patter
    Mutex mutex;
    std::queue<Drawable*> queue;
    ConQueue() {
        pthread_mutex_init(&mutex, NULL);
    }
    void pop() {
        MutexLock lock(mutex);
        queue.pop();
    }
    void push(Drawable *e) {
        MutexLock lock(mutex);
        queue.push(e);
    }

    Drawable &front() {
        MutexLock lock(mutex);
        return *queue.front();
    }
    Drawable &back() {
        MutexLock lock(mutex);
        return *queue.back();
    }
    size_t size() {
        MutexLock lock(mutex);
        return queue.size();
    }
    bool empty() {
        MutexLock lock(mutex);
        return queue.size()==0;
    }
};

struct Synchro
{
    int cntr;
    pthread_barrier_t barrier;
} synchro;
pthread_mutex_t mutex_barrier;
pthread_cond_t cond_barrier_initialized;
#include <cstring>
void runThread(void *arg);

enum Type {CLASS_SHIP, CLASS_CAR_NtoS, CLASS_CAR_StoN};

struct Animable: public Drawable
{
    Animable() {};
    char str[100];
    volatile int posx, posy, fx,tx,fy,ty;
    int width, height;
    int curTime,totalTime;
    int speed;
    bool shouldBeDrawn;
    pthread_t tid;
    Type type;
    void runit() {
        pthreadpool.pthread_create(&tid, (const pthread_attr_t*)NULL, threadFunctionAccessibleFromOutsideWrapper, (void *)this);
        fprintf(stderr, "ran thread %s\n", str);
    }
    Animable *setSpeed(int speed) {this->speed=speed;return this;}
    Animable(Type type, const char* c, int fx, int fy, int tx, int ty, int w=1, int h=1):
        type(type),posx(fx),posy(fy),fx(fx),fy(fy),tx(tx),ty(ty),width(w),height(h)
    {
        snprintf(str, sizeof str, c);
        assert(c);
        fprintf(stderr,"created new %s fx%d tx%d fy%d ty%d w%d h%d\n", str, fx,tx,fy,ty,w,h);
        fflush(stderr);

        curTime=-1;
        shouldBeDrawn=true;
        totalTime=max(abs(tx-fx),abs(ty-fy));
        speed=3;
        //runit();
        //runThread(this);
    };
    void *thread() {
        do
        {
            usleep(1000*1000/this->speed);
            pthread_yield();
            fprintf(stderr, "tickig %s\n", this->str);
        }
        while (this->tick());
        pthreadpool.remove(pthread_self());
        return 0;
    }
    //This is a trick allowing to pass this function as pthread_create_thread parameter, required parameter is pointer to item of this class, because pointer to class method cannot be passed directly to pthread_create_thread
    //http://stackoverflow.com/questions/1151582/pthread-function-from-a-class
    static void *threadFunctionAccessibleFromOutsideWrapper(void *This) {
        fprintf(stderr, "trying to run %s\n", ((Animable*)This)->str);
        return (((Animable*)This)->thread());
    }
    void draw(bool selected=false)
    {
        if(strlen(str))fprintf(stderr, "drawing %s (%d/%d,%d/%d)\n", str, posy, height, posx, width);
        if(shouldBeDrawn)
        {
            for(int i=0; i<width; ++i)
                for(int j=0; j<height; ++j) {
                    mvprintw(posy+j, posx+i, str);//, str[0]);
                    //fprintf(stderr, "drawing (%d/%d,%d/%d)\n", posy+j, height, posx+i, width);
                }
            fprintf(stderr, "drawing %s\n", str);
        }
        else {
            //fprintf(stderr, "not drawing %s\n", str);
        }
    }
    virtual bool tick()
    {
        shouldBeDrawn=true;
        float px=float(posx-fx)/float(tx-fx), py=float(posy-fy)/float(ty-fy);
        //int curTime=min(abs(fx-posx),abs(fy-posy));
        if(curTime!=totalTime)
        {
            curTime++;
            posx=fx+(tx-fx)*curTime/totalTime;
            posy=fy+(ty-fy)*curTime/totalTime;
            fprintf(stderr, "progress x=%f py=%f posx=%d posy=%d cT=%d tlTime=%d t=%s\n", px, py, posx, posy, curTime, totalTime, str);
            fflush(stderr);
        }
        else {
            fprintf(stderr, "%s finished.\n", str);
            fflush(stderr);
            return shouldBeDrawn=false;
        }
        return true;
    }
};

int waterHeight=20;
int roadWidth=6;
struct CarNS: public Animable {
    CarNS(const char *label, int speed=5):
    Animable(CLASS_CAR_NtoS, label, kolumny/2-roadWidth/2+1, 0, kolumny/2-roadWidth/2+1, rzedy-1,2,2) {

    }
    CarNS(char c, int speed=5) {
        char label[100];
        snprintf(label,100,"%c",c);
        CarNS(label, speed);
    };
};
struct CarSN: public Animable {
    CarSN(const char *label, int speed=5):
    Animable(CLASS_CAR_StoN, label, kolumny/2-roadWidth/2+1+2, rzedy-1, kolumny/2-roadWidth/2+1+2, 0,2,2) {

    }
    CarSN(char c, int speed=5) {
        char label[100];
        snprintf(label,100,"%c",c);
        CarSN(label, speed);
    };
};
struct Ship: public Animable {
    Ship(const char *label, int speed=5):
    Animable(CLASS_SHIP, label, kolumny-1, rzedy/2, 0, rzedy/2,20,4) {

    }
};

void *animateThread(void *obj) {
Animable* This=(Animable*)obj;
fprintf(stderr, "trying to run %s\n", ((Animable*)This)->str);
return (This->thread());
}
void runThread(void *arg) {
    pthread_t tid;
    pthreadpool.pthread_create(&tid, (const pthread_attr_t*)NULL, animateThread, (void *)arg);
}

Mutex screenmutex;
Mutex animationmutex;


#define DYNAMICALLY_CREATED_MAX 2000
Animable* dynamically_created[DYNAMICALLY_CREATED_MAX];
void *drawingThread(void *)
{
    while (1)
    {
        //pthread_barrier_init(&synchro.barrier,NULL,synchro.cntr);
        //pthread_cond_broadcast(&cond_barrier_initialized);
        getmaxyx( stdscr, rzedy, kolumny ); //1
        erase();
        attron(COLOR_PAIR(1));//trawa
        for(int y=0; y<rzedy; ++y) for(int x=0; x<kolumny; ++x) mvprintw(y,x," ");
        attroff(COLOR_PAIR(1));
        attron(COLOR_PAIR(2));//woda
        for(int y=rzedy/2-waterHeight/2; y<rzedy/2+waterHeight/2; ++y) for(int x=0; x<kolumny; ++x) mvprintw(y,x,"~");
        attroff(COLOR_PAIR(2));

        for(int i=0; i<DYNAMICALLY_CREATED_MAX; ++i)
        {
            //if(dynamically_created[i].tx!=dynamically_created[i].fx) dynamically_created[i].tick();
            if(dynamically_created[i] && dynamically_created[i]->tx!=dynamically_created[i]->fx) dynamically_created[i]->draw();
        }
        attron(COLOR_PAIR(3));//asfalt
        for(int y=0; y<rzedy; ++y) for(int x=kolumny/2-roadWidth/2; x<kolumny/2+roadWidth/2; ++x) mvprintw(y,x,"~");
        attroff(COLOR_PAIR(3));

        for(int i=0; i<DYNAMICALLY_CREATED_MAX; ++i)
        {
            //if(dynamically_created[i].tx==dynamically_created[i].fx) dynamically_created[i].tick();
            if(dynamically_created[i] && dynamically_created[i]->tx==dynamically_created[i]->fx) dynamically_created[i]->draw();
        }

        attron(COLOR_PAIR(1));//trawa
        move(rzedy-1,kolumny-1);
        attroff(COLOR_PAIR(1));//trawa
        pthread_yield(); //let other threads work
        //pthread_barrier_wait(&synchro.barrier);
        refresh();
        //printf("printifn\n");
        usleep(100*1000);
        //getch();
    }
}
bool paused,once;
int main(int argc, char *argv[])
{
    int rc;
    long t=0;
    initscr();
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_GREEN); //trawa
    init_pair(2, COLOR_BLUE, COLOR_BLUE); //woda
    init_pair(3, COLOR_WHITE, COLOR_WHITE); //asfalt
    stderr=fopen("output.txt", "w+");
    int selected=0;
    while(1)
    {
        pthread_t tid;
        getmaxyx(stdscr, rzedy, kolumny); //1
        synchro.cntr=1;
        int ile=0;
        sleep(1);
        pthread_mutex_init(&screenmutex, NULL);
        pthread_mutex_init(&animationmutex, NULL);
        pthreadpool.pthread_create(&tid, NULL, drawingThread, (void *)NULL);
        runThread(dynamically_created[ile++]=(new CarNS("A"))->setSpeed(5));
        runThread(dynamically_created[ile++]=(new CarSN("B"))->setSpeed(5));
        runThread(dynamically_created[ile++]=(new Ship("C"))->setSpeed(10));
        paused=once=false;
        int k;
        char label[10];
        label[0]='A';label[1]=0;
        while((k=getch())!='r')
        {
            label[0]='A'+ile%('z'-'a');
            switch(k)
            {
            case 'd':
            {
                runThread(dynamically_created[ile++]=(new CarNS(label))->setSpeed(5));
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
                runThread(dynamically_created[ile++]=(new Ship(label))->setSpeed(5));
                fprintf(stderr, "created new ship\n");
                fflush(stderr);
            }
            break;
            case '[':
                do selected--;
                while (selected>1 && !dynamically_created[selected]->shouldBeDrawn);
                fprintf(stderr, "wybrano %d\n", selected);
                fflush(stderr);
                break;
            case ']':
                do selected++;
                while (selected+1<DYNAMICALLY_CREATED_MAX && !dynamically_created[selected]->shouldBeDrawn);
                fprintf(stderr, "wybrano %d\n", selected);
                fflush(stderr);
                break;
            case '=':
                dynamically_created[selected]->speed+=1;
                break;
            case '-':
                if(dynamically_created[selected]->speed>1)dynamically_created[selected]->speed-=1;
                break;
            case 'n':
                once=true;
                break;
            case 'p':
                paused=!paused;
                break;
            }
        };
        perror("RESTARTED");
        pthreadpool.removeAll();
    }
    // endwin();
    /* Last thing that main() should do */
    pthread_exit(NULL);
}
