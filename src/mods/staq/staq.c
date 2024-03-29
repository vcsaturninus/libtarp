#include <tarp/common.h>
#include <tarp/error.h>
#include <tarp/staq.h>

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <tarp/error.h>

#if 0
static inline void stackdump(struct staq *sq){
    assert(sq);
    printf("=== Staq(%zu) [top->bottom] ===\n", sq->count);
    struct staqnode *sqnode = sq->front;
    for (size_t i = 0 ; i < sq->count; i++){
        printf("[%zu] node %p\n", sq->count-1-i, (void*)sqnode);
        sqnode = sqnode->next;
    }
}
#endif

void Staq_init(struct staq *sq, staqnode_destructor dtor){
    *sq = STAQ_INITIALIZER(dtor);
}

struct staq *Staq_new(staqnode_destructor dtor){
    struct staq *sq = (struct staq *)salloc(sizeof(struct staq), NULL);
    *sq = STAQ_INITIALIZER(dtor);
    return sq;
}

bool Staq_empty(const struct staq *sq){
    assert(sq);
    return (sq->count == 0);
}

size_t Staq_count(const struct staq *sq){
    assert(sq);
    return sq->count;
}

struct staqnode *Staq_peekfront(const struct staq *sq){
    assert(sq);
    return sq->front;
}

struct staqnode *Staq_peekback(const struct staq *sq){
    assert(sq);
    return sq->back;
}

/*
 * NOTE on 2 pointer header struct and popping and pushing
 *
 * Notice that the fact that the head struct has 2 pointers (head, tail)
 * instead of just one (head OR tail) does incur a little bit of overhead as
 * two pointers now need to be kept updated when pushing poppping.
 * Specifically: 1) when pushing to an empty list both pointers must be set to
 * point to the new node; 2) when popping from a 1-element list, both pointers
 * must be set to NULL.
 * For queues the 2 pointers are necessary but for stacks they are not, since
 * they could be implemented with a single-pointer header struct.
 * All of this is to say that a singly linked list dedicated to a stack ADT
 * would be faster. Also see
 * (see https://man.freebsd.org/cgi/man.cgi?queue as well).
 * Staq trades off a bit of speed for a little more genericness.
 *
 * ~ ~ ~
 *
 * The 2 pointers in the head structure (struct staq) of the singly-linked list
 * makes possible O(1) access at both ends. However, the list is output-restricted.
 * While nodes can be pushed at either end, they can only be popped at the
 * front of the list. This is a consequence of traversal only being possible
 * front to back but not back to front. Therefore:
 *  - FIFO semantics are implemented through back pushes and front pops. Queues
 *    therefore grow out the back to the right.
 *  - LIFO semantics are impemented through front pushes and front pops. Stacks
 *    therefore grow out the front to the left.
 *  The front of the queue and the top of the stack both coincide with the front
 *  of the underlying linked list.
 */
void Staq_pushback(struct staq *sq, struct staqnode *node){
    assert(sq); assert(node);
    node->next = NULL;

    if (sq->count > 0) sq->back  = sq->back->next = node;
    else               sq->front = sq->back = node;

    sq->count++;
}

/* NOTE popback would be O(n) so is not provided. */
struct staqnode *Staq_popfront(struct staq *sq){
    assert(sq);
    if (sq->count==0) return NULL;
    struct staqnode *node = sq->front;
    sq->front = node->next;
    sq->count--;

    if (sq->count == 0){
        sq->back = NULL;
    }

    return node;
}

void Staq_pushfront(struct staq *sq, struct staqnode *node){
    assert(sq); assert(node);
    node->next = sq->front;

    sq->front = node;
    if (sq->count == 0)  sq->back = node;
    sq->count++;
}

bool Staq_remove(struct staq *sq, bool free_container){
    assert(sq);
    struct staqnode *node = Staq_popfront(sq);

    if (node){
        if (free_container){
            THROWS_ON(sq->dtor==NULL, ERROR_MISCONFIGURED, "missing destructor");
            sq->dtor(node);
        }
        return true;
    }

    return false;
}

void Staq_clear(struct staq *sq, bool free_containers){
    assert(sq);

    if (free_containers){
        while (Staq_remove(sq, free_containers));
    }
    *sq = STAQ_INITIALIZER(sq->dtor); // reset
}

void Staq_destroy(struct staq **sq, bool free_containers){
    assert(sq && *sq);

    Staq_clear(*sq, free_containers);
    salloc(0, *sq);
    *sq = NULL;
}

void Staq_join(struct staq *a, struct staq *b){
    assert(a);
    assert(b);

    if (a->count == 0){
        a->count = b->count;
        a->front = b->front;
        a->back = b->back;
        *b = STAQ_INITIALIZER(b->dtor); /* reset b */

        return;
    }

    a->count += b->count;
    a->back->next = b->front;
    a->back = b->back;
    *b = STAQ_INITIALIZER(b->dtor);
}

void Staq_swap(struct staq *a, struct staq *b){
    assert(a);
    assert(b);

    assert(a); assert(b);
    struct staq tmp = {
        .count = a->count,
        .front = a->front,
        .back = a->back,
        .dtor = a->dtor
    };

    a->front = b->front;
    a->back = b->back;
    a->count = b->count;
    a->dtor = b->dtor;

    b->front = tmp.front;
    b->back  = tmp.back;
    b->count = tmp.count;
    b->dtor = a->dtor;
}

/*
 * Find the (n mod Staq_count())-th node and return it.
 * - The staq must be non-empty.
 * - n is 1-based; n must be > 0
 *
 * The search is done front/top to back/bottom.
 */
static inline struct staqnode *find_nth(const struct staq *sq, size_t n){
    assert(sq);
    assert(sq->count > 0);
    assert(n>0);

    n = n % sq->count; /* if out of bounds, wrap around */

    struct staqnode *node = sq->front;
    while (n > 0){
        node = node->next;
        assert(node);
        --n;
    }

    assert(node);
    return node;
}

void Staq_rotate(struct staq *sq, int dir, size_t num_rotations){
    assert(sq);

    assert(dir == 1 || dir == -1);
    if (dir == 0) return; // invalid

    if (sq->count < 2 || num_rotations == 0) return; // nothing to do

    size_t remainder = num_rotations % sq->count;

    /* 360 degree rotation, no effect, stack would be left unchanged; */
    if (remainder == 0) return;

    /* sq->count rotations does nothing as the stack ends up unchanged;
     * so we can discount multiples of sq->count and only need to do
     * (num_rotations mod sq->count). Since a mod n == (a+kn) mod n. */
    num_rotations = remainder;

    if (dir < 0){
        /* rotate down/to the right/toward the back;
         * the staq is back-restricted and does not offer a way (in O(1)) to get
         * the node to the left of a given node. So true right rotation is not
         * as easily doable (in as efficient a way) as a left-rotate. However,
         * it is not essential. Rotating right n times is equivalent to rotating
         * left (sq->count - n) times. Of course, for a huge stack, this is
         * still relatively inefficient compared to a true right-rotate as
         * more of it than needed has to be traversed. Given this, is it even
         * worth providing a right-rotate rather than only a left-rotate?
         * Yes, for convenience. For example, putting the back node at the
         * front is as simple as Staq_rotate(sq, 1, 1) with right
         * rotations, without needing to consult the length. */
        num_rotations = sq->count - num_rotations; // num rotations to the *left*
    }

    /* instead of rotating node by node, in a loop (and therefore adjusting
     * pointers in a loop), which would be quite slow, a more efficient way
     * of doing it is:
     * - if n=num_rotations, find the (n-1)th node in the staq.
     * - make the (n-1)th the tail of the queue and the nth node the head.
     * This is equivalent to rotating the staq n times. */
    size_t n = num_rotations-1;
    struct staqnode *pre = (n==0) ? sq->front : find_nth(sq, n);
    assert(pre);
    sq->back->next = sq->front; /* close circle, tail moves */
    sq->back = pre;
    sq->front = pre->next;
    pre->next = NULL;
}

// turn 12345 into 54321
void Staq_upend(struct staq *sq){
    assert(sq);
    size_t count = sq->count;
    if (count < 2) return;

    struct staqnode *a, *b, *c;
    a = b = c = sq->front;
    while (a && a->next){
        b = a->next;
        a->next = b->next;
        b->next = c;
        c = b;
    }

    sq->back = a;
    sq->front = b;
}

// insert node after x
void Staq_put_after(
        struct staq *sq,
        struct staqnode *x,
        struct staqnode *node)
{
    assert(sq);
    assert(x);
    assert(node);
    assert(sq->count > 0);

    node->next = x->next;
    x->next = node;
    if (x == sq->back) sq->back = node;
    sq->count++;
}

