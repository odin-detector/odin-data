/*
 * WorkQueue.h
 *
 *  Created on: 26 May 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_WORKQUEUE_H_
#define TOOLS_FILEWRITER_WORKQUEUE_H_

#include <cstddef>
#include <list>

namespace FrameProcessor
{

/** Thread safe producer consumer work queue.
 *
 * This is a thread safe producer consumer queue for use across multiple threads
 * of execution.  Producers can add items to the queue, and consumers block on the
 * arrival of new items.  This WorkQueue is used for transfer of Frame objects
 * between plugins.  Note that the queue is used for transferring pointers to the
 * Frame objects, and not the Frame objects themselves.
 */
  template <typename T> class WorkQueue
  {
    /** Queue (list) of worker items queued for processing */
    std::list<T> m_queue;
    /** Mutex for locking the queue */
    pthread_mutex_t  m_mutex;
    /** Condition for waking up blocked threads when a new item is added to the queue */
    pthread_cond_t   m_condv;

  public:

    /** Constructor.
     *
     * The constructor initilises the mutex and condition required for the class.
     */
    WorkQueue()
    {
      pthread_mutex_init(&m_mutex, NULL);
      pthread_cond_init(&m_condv, NULL);
    }

    /** Destructor.
     *
     * The destructor frees resources (mutex and condition).
     */
    virtual ~WorkQueue()
    {
      pthread_mutex_destroy(&m_mutex);
      pthread_cond_destroy(&m_condv);
    }

    /** Add an item to the queue.
     *
     * The item is added to the list, and then any threads
     * waiting for notification of a new item are signalled.
     *
     * \param[in] item - the item to add to the queue.
     */
    void add(T item)
    {
      pthread_mutex_lock(&m_mutex);
      m_queue.push_back(item);
      pthread_cond_signal(&m_condv);
      pthread_mutex_unlock(&m_mutex);
    }

    /** Remove an item from the queue.
     *
     * Calling this method blocks the current thread until a
     * new item is available in the queue.  If the queue already
     * has items in it then the first one is returned.
     *
     * \return the first item in the queue.
     */
    T remove()
    {
      pthread_mutex_lock(&m_mutex);
      while (m_queue.size() == 0) {
        pthread_cond_wait(&m_condv, &m_mutex);
      }
      T item = m_queue.front();
      m_queue.pop_front();
      pthread_mutex_unlock(&m_mutex);
      return item;
    }

   /** Return the size of the queue.
    *
    * \return the size of the queue.
    */
   int size()
    {
      pthread_mutex_lock(&m_mutex);
      int size = m_queue.size();
      pthread_mutex_unlock(&m_mutex);
      return size;
    }

  };

} /* namespace FrameProcessor */

#endif /* TOOLS_FILEWRITER_WORKQUEUE_H_ */
