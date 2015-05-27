#include <iostream>
#include <queue>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <cstdio>

#include <unistd.h>
#include <signal.h>
#include <errno.h>

using namespace std;

void throw_last_error(const char* message) {
    char err_msg[1024];
    char buffer[1024];
    strerror_r(errno, err_msg, sizeof(buffer));

    sprintf(buffer, "%s (%s)", message, err_msg);
    throw runtime_error(buffer);
}

class Thread {
public:
    Thread():_tid(0),_started(false){}

    void start() {
        if (_started) throw runtime_error("cannot start a thread twice");

        if ( pthread_create(&_tid, NULL, _thread_main_, this) != 0) {
            throw_last_error("cannot create a thread");
        }

        _started = true;
    }

    void* join() {
        void* retval;
        if ( pthread_join(_tid, &retval) != 0) {
            throw_last_error("failed to join a thread");
        }
        return retval;
    }

    void cancel() {
        if ( pthread_cancel(_tid) != 0) {
            throw_last_error("failed to cancel a thraed");
        }
    }

    bool is_running() {
        if (!_started) return false;

        int status = pthread_kill(_tid, 0);
        if (status == 0) return true;
        if (status == ESRCH) return false;
        
        throw_last_error("error calling pthread_kill");
    }

protected:
    virtual void* run() = 0;

private:
    pthread_t _tid;
    bool _started;

    static void* _thread_main_(void* arg) {
        return ((Thread*)arg)->run();
    }
};

template<typename T>
class Worker: public Thread {
public:
    Worker():_lock(PTHREAD_MUTEX_INITIALIZER), _cond(PTHREAD_COND_INITIALIZER){};

    void post(const T& event) {
        push(event);
    }
   
private:
    queue<T> _queue;
    pthread_mutex_t _lock;
    pthread_cond_t _cond;

    virtual void* run() {
        
        while(true) {
            T task = pop();
            task();
        }
        return nullptr;
    }

    T pop() {
        pthread_mutex_lock(&_lock);
        while(_queue.empty()) pthread_cond_wait(&_cond, &_lock);
        T front = _queue.front();
        _queue.pop();
        pthread_mutex_unlock(&_lock);

        return front;
    }

    void push(const T& event) {
        pthread_mutex_lock(&_lock);
        _queue.push(event);
        pthread_cond_signal(&_cond);
        pthread_mutex_unlock(&_lock);
    }
};

typedef void (*Task)();

void print(){
    cout << "hello" << endl;
}

class Deck {
public:
    void operator()() {
        cout << "hello deck" << endl;
    }
};

int main() {
    Worker<Deck> w;
    w.start();
    w.post(Deck());

    w.join();
}
