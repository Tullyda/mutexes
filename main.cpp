#include <iostream>
#include "nanosleep.hpp"
#include <thread>
#include <chrono>
#include <system_error>
#include <mutex>

using namespace std;

class cMutex : public std::mutex
// controlled mutex
// - can be turned on and off
{   
    public:
        enum Status {OFF, ON};
        
        cMutex(Status w = ON) : working(w) { }
        cMutex(bool w = true) : working( (w) ? ON : OFF ) { }
        
        virtual ~cMutex() { }
        
        void lock() {
            if(working) std::mutex::lock();
        }

        void wait() { 
            lock(); 
            std::mutex::unlock();
        }

        cMutex& halt(){ //  przy mutexch synchronizujacych zatrzymuje mutex
            lock();     //  a watek wywolujacy czeka na zwolnienie [ wait() ]
            return *this;
        }

        void release() {
            std::mutex::unlock();
        }

        void activate() {
            working = ON;
        }

        void deactivate() {
            std::mutex::unlock();
            working = OFF;
        }
 

    private:
        Status working;    

        cMutex(const cMutex&) = delete;
        cMutex& operator= (const cMutex&) = delete;
};

/*
class coStream
{
   public:
        coStream(std::ostream& _os) : os(_os) { };
        virtual ~coStream() { }    

        template<typename T> 
        coStream& operator<< (T obj) {
            mtx.lock();
    	    os << obj;
            mtx.unlock();
            return *this;
        }

        void endl() {
            os << std::endl;
        }

   private:
        std::ostream& os;
        std::mutex mtx;
        
        coStream(const coStream&) = delete;
        coStream& operator= (const coStream&) = delete; 
   
};
*/


void delay_thread(int n, cMutex& mtx_stdout, cMutex& mtx_sync)
{
    // inicjalizacja - blokowana mutexem mtx_sync
    mtx_stdout.lock();
        std::cout << "new thread: sleeping for " << n << " seconds" << std::endl;    
    mtx_stdout.unlock();
    mtx_sync.release(); // inicjalizacja zakonczona
    
    // czesc wlasciwa funkcji
    this_thread::sleep_for(chrono::milliseconds(n));

    mtx_stdout.lock();
        std::cout << n << " seconds ended." << std::endl;
    mtx_stdout.unlock();
}



int main(int argc, char *argv[])
{
    cMutex mtx_sync(cMutex::ON); // mutex potrzebny do inicializacji watkow
    cMutex mtx_stdout(cMutex::ON);
// nowy komentarz 2
    try{
        // Wazne by refenrencje przekazac za pomoca std::ref()
        // halt() czeka na zakonczenie inicjalizacji popredniego watku
        // - komunikaty poczatkowe sa wyswietlane w kolejnosci uruchamiania watkow
        thread t1(delay_thread, 3, ref(mtx_stdout), ref(mtx_sync.halt()) );
        thread t2(delay_thread, 5, ref(mtx_stdout), ref(mtx_sync.halt()) ); 
        thread t3(delay_thread, 7, ref(mtx_stdout), ref(mtx_sync.halt()) );
       
        mtx_sync.wait(); // tu by sie przydaly jakies grupy mutexow - group.wait()
        mtx_stdout.lock();
            cout << "now waitnig for the threads to end... " << endl;
        mtx_stdout.unlock();

        t1.join();
        t2.join();
        t3.join();   
    }
    catch (system_error& err){
        cout << "Caught exception: " << err.what() 
             << "/ code: " << err.code() << endl;
    }
    catch (...){
        cout << "Something bad happened!" << endl;
    }

	return 0;
}
