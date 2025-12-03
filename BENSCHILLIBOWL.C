#include "BENSCHILLIBOWL.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>

bool IsEmpty(BENSCHILLIBOWL* bcb);
bool IsFull(BENSCHILLIBOWL* bcb);
void AddOrderToBack(Order **orders, Order *order);

MenuItem BENSCHILLIBOWLMenu[] = { 
    "BensChilli", 
    "BensHalfSmoke", 
    "BensHotDog", 
    "BensChilliCheeseFries", 
    "BensShake",
    "BensHotCakes",
    "BensCake",
    "BensHamburger",
    "BensVeggieBurger",
    "BensOnionRings",
};
int BENSCHILLIBOWLMenuLength = 10;

/* Select a random item from the Menu and return it */
MenuItem PickRandomMenuItem() {
    int index = rand() % BENSCHILLIBOWLMenuLength;
    return BENSCHILLIBOWLMenu[index];
}

/* Allocate memory for the Restaurant, then create the mutex and condition variables needed to instantiate the Restaurant */

BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {
    BENSCHILLIBOWL *bcb = (BENSCHILLIBOWL *)malloc(sizeof(BENSCHILLIBOWL));
    if (bcb == NULL) {
        perror("Failed to allocate BENSCHILLIBOWL");
        exit(1);
    }

    bcb->orders = NULL;
    bcb->current_size = 0;
    bcb->max_size = max_size;
    bcb->next_order_number = 1;
    bcb->orders_handled = 0;
    bcb->expected_num_orders = expected_num_orders;

    // Seed RNG once
    srand(time(NULL));

    pthread_mutex_init(&bcb->mutex, NULL);
    pthread_cond_init(&bcb->can_add_orders, NULL);
    pthread_cond_init(&bcb->can_get_orders, NULL);

    printf("Restaurant is open!\n");
    return bcb;
}


/* check that the number of orders received is equal to the number handled (ie.fullfilled). Remember to deallocate your resources */

void CloseRestaurant(BENSCHILLIBOWL* bcb) {
    // All expected orders should have been handled
    assert(bcb->orders_handled == bcb->expected_num_orders);
    // Also, no pending orders in the queue
    assert(IsEmpty(bcb));

    // Free any remaining orders just in case
    Order *curr = bcb->orders;
    while (curr != NULL) {
        Order *tmp = curr;
        curr = curr->next;
        free(tmp);
    }

    pthread_mutex_destroy(&bcb->mutex);
    pthread_cond_destroy(&bcb->can_add_orders);
    pthread_cond_destroy(&bcb->can_get_orders);

    printf("Restaurant is closed!\n");
    free(bcb);
}

/* add an order to the back of queue */
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {
    pthread_mutex_lock(&bcb->mutex);

    // Wait until there is space in the restaurant
    while (IsFull(bcb)) {
        pthread_cond_wait(&bcb->can_add_orders, &bcb->mutex);
    }

    // Populate order info
    order->order_number = bcb->next_order_number++;
    order->next = NULL;

    // Add to back of queue
    AddOrderToBack(&bcb->orders, order);
    bcb->current_size++;

    // Signal that an order is available
    pthread_cond_signal(&bcb->can_get_orders);

    int order_number = order->order_number;

    pthread_mutex_unlock(&bcb->mutex);
    return order_number;
}

/* remove an order from the queue */
Order *GetOrder(BENSCHILLIBOWL* bcb) {
    pthread_mutex_lock(&bcb->mutex);

    // Wait while queue is empty but we still expect more orders
    while (IsEmpty(bcb) && bcb->orders_handled < bcb->expected_num_orders) {
        pthread_cond_wait(&bcb->can_get_orders, &bcb->mutex);
    }

    // If there are no orders left *and* we've handled all expected orders,
    // notify other cooks and return NULL.
    if (IsEmpty(bcb) && bcb->orders_handled >= bcb->expected_num_orders) {
        // Wake any other cooks so they can also exit
        pthread_cond_broadcast(&bcb->can_get_orders);
        pthread_mutex_unlock(&bcb->mutex);
        return NULL;
    }

    // Remove from front of queue
    Order *order = bcb->orders;
    bcb->orders = order->next;
    bcb->current_size--;
    bcb->orders_handled++;

    // If this was the last order ever, wake up other waiting cooks
    if (bcb->orders_handled >= bcb->expected_num_orders && IsEmpty(bcb)) {
        pthread_cond_broadcast(&bcb->can_get_orders);
    }

    // Signal that there is now space to add orders
    pthread_cond_signal(&bcb->can_add_orders);

    pthread_mutex_unlock(&bcb->mutex);
    return order;
}

// Optional helper functions (you can implement if you think they would be useful)
bool IsEmpty(BENSCHILLIBOWL* bcb) {
    return (bcb->current_size == 0);
}

bool IsFull(BENSCHILLIBOWL* bcb) {
    return (bcb->current_size == bcb->max_size);
}

/* this methods adds order to rear of queue */
void AddOrderToBack(Order **orders, Order *order) {
    if (*orders == NULL) {
        *orders = order;
    } else {
        Order *curr = *orders;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = order;
    }
}


