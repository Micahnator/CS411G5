/*
 * elevator clook
 * File Name: clook-iosched.c
 * Team Number: 05
 * Members' Names: Blaine Eakes, Micah Losli, Warren Mui, David Overgaard
 * Description of Changes: blah blah blah blah blah
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

struct clook_data {
	struct list_head queue;
};

static void clook_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

static int clook_dispatch(struct request_queue *q, int force)
{
	struct clook_data *cd = q->elevator->elevator_data;

	if (!list_empty(&cd->queue)) {
		struct request *rq;
		rq = list_entry(cd->queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
		return 1;
	}
	return 0;
}

static void clook_add_request(struct request_queue *q, struct request *rq)
{
	struct clook_data *cd = q->elevator->elevator_data;
	struct list_head* current;//pointer to the current element in the linux linked list
	struct request * temp;//pointer to the request struct pointed to by current

	//check if the list is empty, and if it is, just put the request in the queue
	if (list_empty(&cd->queue))
		list_add_tail(&rq->queuelist, &cd->queue);
	else {//sort through the list and insert the request in the proper place
		list_for_each(current, (&cd->queue) ) {

		}
	}

}

static int clook_queue_empty(struct request_queue *q)
{
	struct clook_data *cd = q->elevator->elevator_data;

	return list_empty(&cd->queue);
}

static struct request *
clook_former_request(struct request_queue *q, struct request *rq)
{
	struct clook_data *cd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &cd->queue)
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
clook_latter_request(struct request_queue *q, struct request *rq)
{
	struct clook_data *cd = q->elevator->elevator_data;

	if (rq->queuelist.next == &cd->queue)
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static void *clook_init_queue(struct request_queue *q)
{
	struct clook_data *cd;

	cd = kmalloc_node(sizeof(*cd), GFP_KERNEL, q->node);
	if (!cd)
		return NULL;
	INIT_LIST_HEAD(&cd->queue);
	return cd;
}

static void clook_exit_queue(struct elevator_queue *e)
{
	struct clook_data *cd = e->elevator_data;

	BUG_ON(!list_empty(&cd->queue));
	kfree(cd);
}

// Implementing additional functions

static void clook_set_request(struct request_queue *q, struct request *rq, gfp_t)
{
	struct clook_data *cd = q->elevator_data;
	
	// Include private fields - elevator_private & elevator_private2
}

static void clook_put_request(struct request *rq)
{
	// your code goes here
}

static struct elevator_type elevator_clook = {
	.ops = {
		.elevator_merge_req_fn		= clook_merged_requests,
		.elevator_dispatch_fn		= clook_dispatch,
		.elevator_add_req_fn		= clook_add_request,
		.elevator_queue_empty_fn	= clook_queue_empty,
		.elevator_former_req_fn		= clook_former_request,
		.elevator_latter_req_fn		= clook_latter_request,
		.elevator_init_fn		= clook_init_queue,
		.elevator_exit_fn		= clook_exit_queue,
		
		// Adding additional functions
		.elevator_set_req_fn 		= clook_set_request,
		.elevator_put_req_fn		= clook_put_request,
	},
	.elevator_name = "clook",
	.elevator_owner = THIS_MODULE,
};

static int __init clook_init(void)
{
	elv_register(&elevator_clook);

	return 0;
}

static void __exit clook_exit(void)
{
	elv_unregister(&elevator_clook);
}

module_init(clook_init);
module_exit(clook_exit);


MODULE_AUTHOR("Jens Axboe");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("No-op IO scheduler");
