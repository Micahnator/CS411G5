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
	static sector_t disk_head = 0;
	int req_found = 0;

	if (!list_empty(&cd->queue)) {
		struct request *to_be;
		list_for_each_entry(to_be, &cd->queue, queuelist) {
			//to_be = list_entry(cd->queue.next, struct request, queuelist);

			if (to_be->bio->bi_sector >= disk_head) {
				disk_head = to_be->bio->bi_sector + to_be->bio->bi_size;
				req_found = 1;
				break;
			}
		}

		if (req_found == 0) {
			to_be = list_entry(cd->queue.next, struct request, queuelist);
			disk_head = to_be->bio->bi_sector + to_be->bio->bi_size;
		}
		list_del_init(&to_be->queuelist);
		elv_dispatch_add_tail(q, to_be);

		printk("[CLOOK] dsp <%u> <%ul>\n", rq_data_dir(rq), rq->bio->bi_sector);

		//print out the state of the queue
		struct request* cur_req;
		int i = 0;
		list_for_each_entry(cur_req, &cd->queue, queuelist){
			printk("[node:%d sector:%lu]\n", i, (unsigned long)(rq_data_dir(cur_req)));
		}	


		return 1;
	}
	return 0;
}

static void clook_add_request(struct request_queue *q, struct request *rq)
{
	//get the request struct
	struct clook_data *cd = q->elevator->elevator_data;
	//pointer to the cur_req element in the linux linked list
	struct request * cur_req;
	sector_t new_request_sector, cur_request_sector, next_request_sector;

	//get the request's sector
	new_request_sector = rq->bio->bi_sector;

	//If the list is empty
	if (list_empty(&cd->queue))
		list_add(&rq->queuelist, &cd->queue);

	//Sort through the list and insert the request in the proper place
	else {
		list_for_each_entry(cur_req, &cd->queue, queuelist) {

			//get the sector of the cur_req and next node
			cur_request_sector = cur_req->bio->bi_sector;

			//If next element is not the head
			if(&cur_req->queuelist != &cd->queue)
				next_request_sector = cur_req->bio->bi_next->bi_sector;
			else {
				struct request * pos = list_entry((&cd->queue)->next, typeof(*pos), queuelist);
				next_request_sector = pos->bio->bi_sector;
			}

			//If the request is one element in the list
			if(cur_request_sector==next_request_sector) {
				if(new_request_sector < cur_request_sector)
					list_add_tail(&rq->queuelist, &cur_req->queuelist);
				else
					list_add(&rq->queuelist, &cur_req->queuelist);
			}
			else if((new_request_sector > cur_request_sector) &&
					(new_request_sector < next_request_sector))
				list_add(&rq->queuelist, &cur_req->queuelist);
			//If new is larger than anything in the list
			else if((new_request_sector > cur_request_sector) &&
					(cur_request_sector > next_request_sector))
				list_add(&rq->queuelist, &cur_req->queuelist);
			//If new is smaller than anything in the list
			else if((new_request_sector < next_request_sector) &&
					(cur_request_sector > next_request_sector))
				list_add(&rq->queuelist, &cur_req->queuelist);
			else if(new_request_sector == cur_request_sector)
				list_add(&rq->queuelist, &cur_req->queuelist);
		}
	}
	printk("[CLOOK] add <%u> <%ul>\n", rq_data_dir(rq), rq->bio->bi_sector);
	return;
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
/*
   static void clook_set_request(struct request_queue *q, struct request *rq, gfp_t)
   {
   struct clook_data *cd = q->elevator_data;

// Include private fields - elevator_private & elevator_private2
}

static void clook_put_request(struct request *rq)
{
// your code goes here
}
 */

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
		//.elevator_set_req_fn 		= clook_set_request,
		//.elevator_put_req_fn		= clook_put_request,
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
