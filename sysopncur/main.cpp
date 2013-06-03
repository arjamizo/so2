#include <ncurses.h>
#include <algorithm>
#include <unistd.h>
#include <pthread.h>
using namespace std;
//zmienne
int kolumny = 0;
int rzedy = 0;
char tekst[] = "Wyswietlany tekst!";

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
    }
};

struct Synchro
{
    int cntr;
    pthread_barrier_t barrier;
} synchro;
pthread_mutex_t mutex_barrier;
pthread_cond_t cond_barrier_initialized;

struct Animable: public Drawable
{
    Animable() {};
    char c;
    char str[2];
    Synchro *synchro;
    int posx, posy, fx,tx,fy,ty;
    int w, h;
    int curTime,totalTime;
    int speed;
    bool shouldBeDrawn;
    Animable(char c, int fx, int fy, int tx, int ty, Synchro &synchro, int w=1, int h=1):
        c(c),posx(fx),posy(fy),fx(fx),fy(fy),tx(tx),ty(ty),w(w),h(h),synchro(&synchro)
    {
        fprintf(stderr,"created new fx%d tx%d fy%d ty%d w%d h%d\n", fx,tx,fy,ty,w,h);
        fflush(stderr);
        str[0]=c;
        str[1]=0;
        curTime=-1;
        shouldBeDrawn=false;
        totalTime=max(abs(tx-fx),abs(ty-fy));
        speed=3;
    };
    virtual void draw()
    {
        if(shouldBeDrawn)
            for(int i=0; i<w; ++i)
                for(int j=0; j<h; ++j)
                    mvprintw(posy+j, posx+i, str);
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
        else return shouldBeDrawn=false;
        return true;
    }
};


#define MAX_OBJS 2000
Animable *objs[MAX_OBJS];
int objcnt=0;

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

struct AnimationThreadArgs
{
    pthread_t id;
    Animable *obj;
    AnimationThreadArgs(pthread_t id, Animable *obj):obj(obj),id(id) {};
};

void *animationThread(void *arg)
{
    //AnimationThreadArgs *animags=(AnimationThreadArgs*)args;
    //Animable *obj=animags->obj;
    Animable *obj=(Animable*)arg;
    pthread_mutex_lock(&mutex_barrier);
    obj->synchro->cntr++;
    pthread_mutex_unlock(&mutex_barrier);
    fprintf(stderr, "i ruszylo!...\n", obj->str);
    fflush(stderr);
    do
    {
        //pthread_mutex_lock(&mutex_barrier);
        //fprintf(stderr, "oczekiwanie %s na init bariery...\n", obj->str);
        //
        //pthread_cond_wait(&cond_barrier_initialized,&mutex_barrier); //nie trzeba sprawdzac whilem dlatego, ze broadcastujemy wszystko

        //pthread_mutex_unlock(&mutex_barrier);
        //pthread_barrier_wait(&obj->synchro.barrier);
        //sleep(1);
        //pthread_ytrakield();
        fprintf(stderr, "waitinf %s\n", obj->str);
        fflush(stderr);
        usleep(1000*1000/obj->speed);

    }
    while (obj->tick());
    pthreadpool.remove(pthread_self());
}

struct Runnable
{
    Runnable(const Animable &obj)
    {
        pthread_t tid;
        pthreadpool.pthread_create(&tid, (const pthread_attr_t*)NULL, animationThread, (void *)&obj);
    }
};

int waterHeight=20;
int roadWidth=6;

Animable dynamically_created[200];
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

        for(int i=0; i<MAX_OBJS; ++i)
        {
            if(objs[i] && objs[i]->tx==objs[i]->fx) objs[i]->draw(); //rysuj tylko samochody
        }
        attron(COLOR_PAIR(3));//asfalt
        for(int y=0; y<rzedy; ++y) for(int x=kolumny/2-roadWidth/2; x<kolumny/2+roadWidth/2; ++x) mvprintw(y,x,"~");
        attroff(COLOR_PAIR(3));

        for(int i=0; i<MAX_OBJS; ++i)
        {
            if(objs[i] && objs[i]->tx!=objs[i]->fx) objs[i]->draw(); //rysuj tylko statki
        }
        for(int i=0; i<200; ++i)
        {
            dynamically_created[i].draw();
        }

        attron(COLOR_PAIR(1));//trawa
        move(rzedy-1,kolumny-1);
        attroff(COLOR_PAIR(1));//trawa
        //pthread_yield(); //let other threads work
        //pthread_barrier_wait(&synchro.barrier);
        refresh();
        //printf("printifn\n");
        usleep(100*1000);
        //getch();
    }
}

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
    while(1)
    {
        getmaxyx( stdscr, rzedy, kolumny ); //1
        synchro.cntr=1;
        Animable car('A' ,kolumny/2-roadWidth/2+1, 0, kolumny/2-roadWidth/2+1, rzedy-1,synchro,2,2);
        Animable car2('B',kolumny/2+roadWidth/2-2-1, rzedy-1  , kolumny/2+roadWidth/2-2-1,0,synchro,2,2);
        Animable ship('S',kolumny-1, 10, 0, 10,synchro,20,4);
        objs[0]=&car;
        objs[1]=&car2;
        objs[2]=&ship;
        car.speed=5;
        car2.speed=10;
        ship.speed=10;
        //pthread_mutex_init(&mutex_barrier, NULL);
        //pthread_cond_init(&cond_barrier_initialized, NULL);
        //fprintf(stderr,"Wszystkie watki zatrzymane. Oczekiwanie na inicjalizacje bariery. \n");fflush(stderr);
        sleep(1);
        pthread_t tid;
        pthreadpool.pthread_create(&tid, NULL, drawingThread, (void *)NULL);
        for(int i=0; i<MAX_OBJS; ++i) if(objs[i])
            {
                pthreadpool.pthread_create(&tid, NULL, animationThread, (void *)objs[i]);
            }
        int k;
        int ile=0;
        while((k=getch())!='r')
        {
            switch(k)
            {
            case 'c':
            {
                dynamically_created[ile++]=Animable('A' ,kolumny/2-roadWidth/2+1,  0, kolumny/2-roadWidth/2+1, rzedy-1                  ,synchro,2,2);
                Runnable runThatThread(dynamically_created[ile-1]);
                fprintf(stderr, "created new car\n");
                fflush(stderr);
            }
            break;
            case 'C':
            {
                dynamically_created[ile++]=Animable('B',kolumny/2+roadWidth/2-2-1, rzedy-1  , kolumny/2+roadWidth/2-2-1,0,synchro,2,2);
                Runnable runThatThread(dynamically_created[ile-1]);
                fprintf(stderr, "created new car2\n");
                fflush(stderr);
            }
            break;
            case 's':
            {
                dynamically_created[ile++]=Animable('S',kolumny-1, rzedy/2, 0, rzedy/2,synchro,20,4);
                Runnable runThatThread(dynamically_created[ile-1]);
                fprintf(stderr, "created new ship\n");
                fflush(stderr);
            }
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
