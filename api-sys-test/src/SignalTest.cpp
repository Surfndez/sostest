#include <sapi/sys.hpp>
#include <sapi/var.hpp>
#include <sapi/chrono.hpp>
#include "SignalTest.hpp"
#include "ThreadTest.hpp"


static volatile bool stop_threads = false;
static const u32 threads_wait_time = 1000;
SignalTest::SignalTest() : Test("sys::Signal"){
   Data::reclaim_heap_space();
}
static volatile bool wait_for_signal = true;
static volatile bool wait_for_associated_signal = true;
static int sig_no_last;
static int sig_value_last;
static bool thread_result = true;
static void common_handler_1(int a);
static void common_handler_2(int a,siginfo_t * siginfo, void * args);
static void * signal_thread(void * args);
/*@brief api test for sys/Signal use "api-sys-test -signal -api"
 * SignalHandler
 *
 * Signal
 * set_handler,signo,sigvalue
 * not writed test
 * Signal,Signal,event,trigger,trigger,trigger,trigger,
 *
 * not tested on
 * suspend if signo = 0
 */
bool SignalTest::execute_class_api_case(){
   bool result = true;
   enum Signal::number signal_no;
   int sig_value;
   SignalHandler handler_1 = SignalHandler(
            Signal::Function(common_handler_1)
            );
   SignalHandler handler_2 = SignalHandler(
            Signal::ActionFunction(common_handler_2),
            Signal::Flags(0),
            Signal::Mask(0)
            );
   Timer timer;
   timer.start();
   {
      sig_value = 1;
      signal_no = Signal::INT;
      Signal signal_send(
               signal_no,
               Signal::ValueInteger(sig_value)
               );
      //this will cause my_handler() to be executed when SIGINT is received
      if(signal_send.signo()!=signal_no){
         print_case_message("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
         result = false;
      }

      TEST_THIS_EXPECT(bool, signal_send.set_handler(handler_1) == 0, true);

      if(signal_send.sigvalue()!= sig_value){
         print_case_message("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
         result = false;
      }

      //the program will stay in this loop until SIGINT is received
      wait_for_signal = true;
      while( wait_for_signal == true ){
         wait(Milliseconds(1));
         signal_send.send();
      }
      if(sig_no_last != signal_no){
         print_case_message("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
         result = false;
      }
   }
   {
      sig_value = 2;
      signal_no = Signal::USR1;
      Signal signal_send(signal_no,Signal::ValueInteger(sig_value));
      wait_for_signal = true;
      if(signal_send.set_handler(handler_1)!=0){
         print_case_message("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
         result = false;
      }
      while( wait_for_signal == true ){
         wait(Milliseconds(1));
         signal_send.send();
      }
      if(sig_no_last != signal_no){
         print_case_message("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
         result = false;
      }
   }
   {
      sig_value = 3;
      signal_no = Signal::USR2;
      Signal signal_send(signal_no,Signal::ValueInteger(sig_value));
      if(signal_send.set_handler(handler_2)!=0){
         print_case_message("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
         result = false;
      }
      wait_for_associated_signal = true ;
      while( wait_for_associated_signal == true ){
         wait(Milliseconds(1));
         signal_send.queue(Sched::get_pid());
      }
      if(sig_no_last != signal_no){
         print_case_message("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
         result = false;
      }
      if(sig_value_last != sig_value){
         print_case_message("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
         result = false;
      }
   }
   //send signal to another thread
   {
      Thread uno_thread(Thread::StackSize(4096));
      int signal_thread_priority = 1;
      enum Sched::policy signal_thread_policy = Sched::RR;



      CREATE_THREAD(uno_thread, signal_thread, this, signal_thread_priority, signal_thread_policy);

      sig_value = 4;
      signal_no = Signal::USR2;
      Signal signal_send(
               signal_no,
               Signal::ValueInteger(sig_value)
               );
      if(signal_send.set_handler(handler_2)!=0){
         print_case_message("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
         result = false;
      }
      wait_for_associated_signal = true ;
      while( wait_for_associated_signal == true ){
         wait(Milliseconds(1));
         signal_send.queue(uno_thread.get_pid());
      }
      if(sig_no_last != signal_no){
         print_case_message("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
         result = false;
      }
      if(sig_value_last != sig_value){
         print_case_message("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
         result = false;
      }
      sig_value = 5;
      signal_no = Signal::ABRT;
      Signal signal_abrt(signal_no,Signal::ValueInteger(sig_value));
      if(signal_abrt.set_handler(handler_2)!=0){
         print_case_message("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
         result = false;
      }
      signal_abrt.send(uno_thread.get_pid());
      wait(Milliseconds(100));
      if(uno_thread.is_running()){
         while(uno_thread.is_running()){
            stop_threads = 1;
            wait(Milliseconds(1));
         }
      }
   }

   print_case_message("timer %d",timer.microseconds());
   return result;
}
/*@brief stress test for sys/Signal use "api-sys-test -signal -stress"
 * not writed test
 * all sig value
 */
bool SignalTest::execute_class_stress_case(){
   bool result = true;
   enum Signal::number signal_no;
   int sig_value;
   SignalHandler handler = SignalHandler(
            Signal::Function(common_handler_1)
            );
   Thread signal(Thread::StackSize(2048));
   enum Sched::policy signal_policy;
   pthread_t signal_pthread;
   pid_t signal_pid;
   signal_policy = Sched::RR;
   const int signal_priority = 1;
   u16 itterate_num = 100;
   u8 itterate_signo_num = 0;
   u8 itterate_sig_value_num = 30;
   Timer timer;
   wait_for_signal = true;
   timer.start();
   //signo test
   Thread uno_thread(Thread::StackSize(4096));
   enum Sched::policy signal_thread_policy = Sched::RR;
   int signal_thread_priority = Sched::get_priority_max(Sched::RR);

   CREATE_THREAD(uno_thread, handle_thread_1, this, signal_thread_priority, signal_thread_policy);

   for (int j=0;j<itterate_num;j++){
      for (int i=0;i<itterate_signo_num;i++){
         //this will cause my_handler() to be executed when SIGINT is received
         signal_no = get_signal_number(i%31);
         sig_value = 100;
         Signal event(signal_no,Signal::ValueInteger(sig_value));
         TEST_THIS_EXPECT(int, event.signo(), signal_no);
         TEST_THIS_EXPECT(int, event.sigvalue(), sig_value);
         TEST_THIS_EXPECT(int, event.set_handler(handler), 0);
         //the program will stay in this loop until SIGINT is received
         while( wait_for_signal == true ){
            wait(Milliseconds(1));
            event.send();
         }
         TEST_THIS_EXPECT(int, sig_no_last, sig_no_last);
         wait_for_signal = true;
      }
      //sig_value test
      for (int i=0;i<itterate_sig_value_num;i++){
         //this will cause my_handler() to be executed when SIGINT is received
         signal_no = get_signal_number(1);
         sig_value = i;
         Signal event(signal_no,Signal::ValueInteger(sig_value));
         TEST_THIS_EXPECT(int, event.signo(), signal_no);
         TEST_THIS_EXPECT(int, event.sigvalue(), sig_value);
         TEST_THIS_EXPECT(int, event.set_handler(handler), 0);

         //the program will stay in this loop until SIGINT is received
         while( wait_for_signal == true ){
            wait(Milliseconds(1));
            event.send();
         }
         wait_for_signal = true;
      }
      //send signal to another thread
      {
         SignalHandler handler_2 = SignalHandler(
                  Signal::ActionFunction(common_handler_2)
                  );

         sig_value = rand();
         u8 rand_value;
         do {
            rand_value = rand()%31;
            signal_no = get_signal_number(rand_value);
            //kill and stop cannot be handled
         } while( signal_no == Signal::KILL || signal_no == Signal::STOP );

         Signal signal_send(signal_no,Signal::ValueInteger(sig_value));

         TEST_THIS_EXPECT(int, signal_send.set_handler(handler_2), 0);
         wait_for_associated_signal = true ;
         while( wait_for_associated_signal == true ){
            wait(Milliseconds(1));
            signal_send.queue(uno_thread.get_pid());
         }

         TEST_THIS_EXPECT(int, sig_no_last, signal_no);
         TEST_THIS_EXPECT(int, sig_value_last, sig_value);
      }
   }
   if(uno_thread.is_running()){
      while(uno_thread.is_running()){
         stop_threads = 1;
         wait(Milliseconds(1));
      }
   }

   CREATE_THREAD(signal, signal_thread, this, signal_priority, signal_policy);

   sig_value = Signal::ABRT;
   //Signal abort_sig(get_signal_number(1), Signal::ValueInteger(sig_value));
   signal_pthread = signal.id();
   signal_pid = signal.get_pid();
   wait(Microseconds(threads_wait_time));
   print_case_message("signal_pthread %d,signal_pid %d",signal_pthread,signal_pid);
   print_case_message("timer %d",timer.microseconds());
   return result;
}

/*@brief performance test for sys/Signal use "api-sys-test -signal -performance"
 */
bool SignalTest::execute_class_performance_case(){
   //this test needs to measure how fast signals are sent and received
   return case_result();
}

void * SignalTest::thread_1(u32 wait_time){
   int counter = 0;
   (void)wait_time;
   SignalHandler handler_2 = SignalHandler(
            Signal::ActionFunction(common_handler_2)
            );
   Signal incoming_signal(Signal::USR2);
   incoming_signal.set_handler(handler_2);
   while( !stop_threads ){
      counter++;
      wait(Microseconds(threads_wait_time*1000));
      //add data performance test from api-var-test(make it more difficult)
      u32 data_size = (rand() & 0xfff) + 1; //12 bits is up to 4096
      Data data(data_size);
      if( data.data() == nullptr){
         print_case_message("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
         thread_result = false;
         break;
      }
      char buffer[data_size];
      memset(buffer, 0xaa, data_size);
      data.fill(0xaa);
      if( memcmp(buffer, data.to_const_void(), data_size) ){
         print_case_message("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
         thread_result = false;
         break;
      }
      memset(buffer, 0x00, data_size);
      //add clear test
      data.clear();
      if( memcmp(buffer, data.to_const_void(), data_size) ){
         print_case_message("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
         thread_result = false;
         break;
      }
      if (data_size){
         char* t;
         t = data.to_char();
         //change one byte in data
         t[data_size-1] = 0x0e;
         if( !memcmp(buffer, data.to_const_void(), data_size) ){
            print_case_message("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
            thread_result = false;
            break;
         }
      }
   }
   return &thread_result;
}
static void * signal_thread(void * args){
   int counter = 0;
   SignalHandler handler_2 = SignalHandler(
            Signal::ActionFunction(common_handler_2)
            );
   Signal incoming_signal(Signal::USR2,0);
   incoming_signal.set_handler(handler_2);
   while( !stop_threads ){
      counter++;
      wait(Microseconds(threads_wait_time*1000));
      //add data performance test from api-var-test
      u32 data_size = (rand() & 0xfff) + 1; //12 bits is up to 4096
      Data data(data_size);
      if( data.data() == nullptr ){
         thread_result = false;
         break;
      }
      char buffer[data_size];
      memset(buffer, 0xaa, data_size);
      data.fill(0xaa);
      if( memcmp(buffer, data.to_const_void(), data_size) ){
         thread_result = false;
         break;
      }
      memset(buffer, 0x00, data_size);
      //add clear test
      data.clear();
      if( memcmp(buffer, data.to_const_void(), data_size) ){
         thread_result = false;
         break;
      }
      if (data_size){
         char* t;
         t = data.to_char();
         //change one byte in data
         t[data_size-1] = 0x0e;
         if( !memcmp(buffer, data.to_const_void(), data_size) ){
            thread_result = false;
            break;
         }
      }
   }
   return args;
}
static void common_handler_1(int a){
   //do something
   (void)a;
   sig_no_last = a;
   wait_for_signal = false;
}
static void common_handler_2(int a,siginfo_t * siginfo, void * args){
   (void)a;
   (void)siginfo;
   (void)args;
   sig_no_last = a;
   sig_value_last = siginfo->si_value.sival_int;
   wait_for_associated_signal = false;
}
enum Signal::number SignalTest::get_signal_number(int value){
   switch (value){
      case 0:
         return Signal::ABRT;
      case 1:
         return Signal::ALRM;
      case 2:
         return Signal::BUS;
      case 3:
         return Signal::CHLD;
      case 4:
         return Signal::CONT;
      case 5:
         return Signal::FPE;
      case 6:
         return Signal::HUP;
      case 7:
         return Signal::ILL;
      case 8:
         return Signal::INT;
      case 9:
         return Signal::KILL;
      case 10:
         return Signal::PIPE;
      case 11:
         return Signal::QUIT;
      case 12:
         return Signal::SEGV;
      case 13:
         return Signal::STOP;
      case 14:
         return Signal::TERM;
      case 15:
         return Signal::TSTP;
      case 16:
         return Signal::TTIN;
      case 17:
         return Signal::TTOU;
      case 18:
         return Signal::USR1;
      case 19:
         return Signal::USR2;
      case 20:
         return Signal::POLL;
      case 21:
         return Signal::PROF;
      case 22:
         return Signal::SYS;
      case 23:
         return Signal::TRAP;
      case 24:
         return Signal::URG;
      case 25:
         return Signal::TALRM;
      case 26:
         return Signal::XCPU;
      case 27:
         return Signal::XFSZ;
      case 28:
         return Signal::RTMIN;
      case 29:
         return Signal::RT;
      case 30:
         return Signal::RTMAX;
      default:
         return Signal::USR2;
   }
}
