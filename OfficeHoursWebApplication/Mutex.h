#pragma once

// A simple mutex (mutual exclusion) mechanism.

#include <pthread.h>
#include <semaphore.h>

class Mutex
   {
   private:
      pthread_mutex_t lock;

   public:
      void Lock( )
         {
         pthread_mutex_lock( &lock );
         }
      
      void Unlock( )
         {
         pthread_mutex_unlock( &lock );
         }

      Mutex( )
         {
         pthread_mutex_init( &lock, nullptr );
         }

      ~Mutex( )
         {
         pthread_mutex_destroy( &lock );
         }
   };