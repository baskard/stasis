/*---
This software is copyrighted by the Regents of the University of
California, and other parties. The following terms apply to all files
associated with the software unless explicitly disclaimed in
individual files.

The authors hereby grant permission to use, copy, modify, distribute,
and license this software and its documentation for any purpose,
provided that existing copyright notices are retained in all copies
and that this notice is included verbatim in any distributions. No
written agreement, license, or royalty fee is required for any of the
authorized uses. Modifications to this software may be copyrighted by
their authors and need not follow the licensing terms described here,
provided that the new terms are clearly indicated on the first page of
each file where they apply.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
NON-INFRINGEMENT. THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, AND
THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

GOVERNMENT USE: If you are acquiring this software on behalf of the
U.S. government, the Government shall have only "Restricted Rights" in
the software and related documentation as defined in the Federal
Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2). If you are
acquiring the software on behalf of the Department of Defense, the
software shall be classified as "Commercial Computer Software" and the
Government shall have only "Restricted Rights" as defined in Clause
252.227-7013 (c) (1) of DFARs. Notwithstanding the foregoing, the
authors grant the U.S. Government and others acting in its behalf
permission to use and distribute the software in accordance with the
terms specified in this license.
---*/
/**
 * @file
 * Manages the page buffer

    pageManager - Provides cached page handling, delegates to blob
    manager when necessary.  Doesn't implement an eviction policy.
    That is left to a cacheManager.  (Multiple cacheManagers can be
    used with a single page manager.)

 
  @todo Allow error checking!  
 
  @todo Make linux provide a better version of malloc().  We need to
  directly DMA pages into and out of userland, or setup mmap() so
  that it takes a flag that makes it page mmapped() pages to swap
  instead of back to disk. (munmap() and msync() would still hit the
  on-disk copy)
 
  @todo Refactoring for lock manager
 
  Possible interface for lockManager:

       Define three classes of objects that the lock manager is interested in:

         Transactions,
	 Operations,
	 Predicates.

       LLADD already has operations and transactions, and these can be
       relatively unchanged.  Predicates are read only operations that
       return a set of tuples.  Tread() is the simplest predicate.
       Index scans provide a motivating example.  

       See http://research.microsoft.com/%7Eadya/pubs/icde00.pdf
       (Generalized Isolation Level Definitions, Adya, Liskov, O'Neil,
       2000) for a theoretical discussion of general locking schemes..

       Locking functions can return errors such as DEADLOCK, etc.
       When such a value is returned, the transaction aborts, and an
       error is passed up to the application.

 * @ingroup LLADD_CORE
 * $Id$
 */

#ifndef __BUFFERMANAGER_H__
#define __BUFFERMANAGER_H__

#include <lladd/page.h>
#include <lladd/constants.h>

/**
 * initialize buffer manager
 * @return 0 on success
 * @return error code on failure
 */
int bufInit();

/**
 * @param pageid ID of the page you want to load
 * @return fully formed Page type
 * @return page with -1 ID if page not found
 */
Page loadPage(int pageid); 

/**
 * allocate a record
 * @param xid The active transaction.
 * @param size The size of the new record
 * @return allocated record
 */
recordid ralloc(int xid, long size);

/**
 * Find a page with some free space.
 *
 */
 

/* *
 * This function updates the LSN of a page.
 * 
 * This is needed by the
 * recovery process to make sure that each action is undone or redone
 * exactly once.
 *
 * @ param LSN The new LSN of the page.
 * @ param pageid ID of the page you want to write
 *
 * @ todo This needs to be handled by ralloc and writeRecord for
 * correctness.  Right now, there is no way to atomically update a
 * page(!)  To fix this, we need to change bufferManager's
 * implementation to use read/write (to prevent the OS from stealing
 * pages in the middle of updates), and alter kickPage to see what the
 * last LSN synced to disk was.  If the log is too far behind, it will
 * need to either choose a different page, or call flushLog().  We may
 * need to implement a special version of fwrite() to do this
 * atomically.  (write does not have to write all of the data that was
 * passed to it...)
 */
/*void writeLSN(long LSN, int pageid);  */

/**
 * @param pageid ID of page you want to read
 * @return LSN found on disk
 */
long readLSN(int pageid);

/**
 * @param xid transaction id @param lsn the lsn that the updated
 * record will reflect.  This is needed by recovery, and undo.  (The
 * lsn of a page must always increase.  Undos are handled by passing
 * in the LSN of the CLR that records the undo.)
 *
 * @param rid recordid where you want to write @param dat data you
 * wish to write
 */
void writeRecord(int xid, lsn_t lsn, recordid rid, const void *dat);

/**
 * @param xid transaction ID
 * @param rid
 * @param dat buffer for data
 */
void readRecord(int xid, recordid rid, void *dat);

/**
 * Write page to disk, including correct LSN.  Doing so may require a
 * call to logSync().  There is not much that can be done to avoid
 * this call right now.  In the future, it might make sense to check
 * to see if some other page can be kicked, in order to avoid the log
 * flush.  
 *
 * @param dat  The page to be flushed to disk.
 */
void pageWrite(const Page * dat);


/**
   Read a page from disk.  

   @param ret A page struct, with id set correctly.  The rest of this
   struct will be overwritten by pageMap.
*/
void pageRead(Page * ret);


/* int flushPage(Page page); */

/*void pageMap(Page * page); */
/*
 * this function does NOT write to disk, just drops the page from the active
 * pages
 * @param page to take out of buffer manager
 * @return 0 on success
 * @return error code on failure
int dropPage(Page page);
 */

/**
 * all actions necessary when committing a transaction. Can assume that the log
 * has been written as well as any other actions that do not depend on the
 * buffer manager
 *
 * Basicly, this call is here because we used to do copy on write, and
 * it might be useful when locking is implemented.
 *
 * @param xid transaction ID
 * @param lsn the lsn at which the transaction aborted.  (Currently
 * unused, but may be useful for other implementations of the buffer
 * manager.)
 * @return 0 on success
 * @return error code on failure
 */
int bufTransCommit(int xid, lsn_t lsn);

/**
 * 
 * Currently identical to bufTransCommit.
 * 
 * @param xid transaction ID 
 * 
 * @param lsn the lsn at which the transaction aborted.  (Currently
 * unused, but may be useful for other implementations of the buffer
 * manager.)
 *
 * @return 0 on success
 *  
 * @return error code on failure
 */
int bufTransAbort(int xid, lsn_t lsn);

/**
 * will write out any dirty pages, assumes that there are no running
 * transactions
 */
void bufDeinit();

#endif
