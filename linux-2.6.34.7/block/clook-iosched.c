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

//Function prototypes
static struct request *
clook_latter_request(struct request_queue *q, struct request *rq);

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

		printk("[CLOOK] dsp R/W: <%u> Sector: <%lu>\n", rq_data_dir(to_be), to_be->bio->bi_sector);

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
	struct request * pos;
	sector_t cur_request_sector=5, next_request_sector=NULL, new_request_sector=NULL; 

	//get the request's sector
	new_request_sector = rq->bio->bi_sector;

		int flag=0;
		list_for_each_entry(cur_req, &cd->queue, queuelist) {
		
		//cur_req = clook_latter_request(q, &cd->queue);
		if(cur_req != NULL)
		{
			cur_request_sector = cur_req->bio->bi_sector;
			pos = clook_latter_request(q, cur_req); //list_entry((&cd->queue)->next, typeof(*pos), queuelist);
			if(pos)
				next_request_sector = pos->bio->bi_sector;
		}



			printk("NEW SECTOR #:<%lu>, CURRENT SECTOR #:<%lu>, NEXT SECTOR #: <%lu>\n",
                                   new_request_sector, cur_request_sector, next_request_sector);


			//If the request is one element in the list
			if(cur_request_sector==next_request_sector) {
				if(new_request_sector < cur_request_sector)
					list_add_tail(&rq->queuelist, &cur_req->queuelist);
				else
					list_add(&rq->queuelist, &cur_req->queuelist);
				flag=1;
				break;
			}
			else if((new_request_sector > cur_request_sector) &&
					(new_request_sector < next_request_sector)) {
				list_add(&rq->queuelist, &cur_req->queuelist);
				flag=1;
				break;
			}
			//If new is larger than anything in the list
			else if((new_request_sector > cur_request_sector) &&
					(cur_request_sector > next_request_sector)) {
				list_add(&rq->queuelist, &cur_req->queuelist);
				flag=1;
				break;
			}
			//If new is smaller than anything in the list
			else if((new_request_sector < next_request_sector) &&
					(cur_request_sector > next_request_sector)) {
				list_add(&rq->queuelist, &cur_req->queuelist);
				flag=1;
				break;
			}
			else if(new_request_sector == cur_request_sector) {
				list_add(&rq->queuelist, &cur_req->queuelist);
				flag=1;
				break;
			}
		}

		//If the list is empty
		if (flag==0)
			list_add(&rq->queuelist, &cd->queue);

		//	}
		printk("[CLOOK] add R/W: <%u> Sector: <%lu>\n", rq_data_dir(rq), rq->bio->bi_sector);
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
