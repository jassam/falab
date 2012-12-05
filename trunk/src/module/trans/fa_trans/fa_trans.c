#include "fa_trans.h"
#include "fa_malloc.h"
#include "fa_print.h"
#include "fa_network.h"


static fa_trans_func_ext_t trans_func_ext = {
	NULL,	//ext1: for exp: seek function,need to be filled in the child function
};

fa_trans_t * fa_create_trans(int (* create_trans_callback)(fa_trans_t *trans),
                             int (* destroy_trans_callback)(fa_trans_t *trans))
{
	int ret;

    fa_trans_t *trans = fa_malloc(sizeof(fa_trans_t));


	trans->open = NULL;
	trans->send = NULL;
	trans->recv = NULL;
	trans->close = NULL;
	trans->trans_name = NULL;
	trans->priv_data = NULL;
	trans->func_ext = &trans_func_ext;
    trans->destroy_trans = NULL;


    ret = create_trans_callback(trans);
    if(ret)
        goto fail;

    trans->destroy_trans = destroy_trans_callback;

    return trans;	//return the addr of the create trans

fail:
	return NULL;		//return 0 , means not create successfully
}


int fa_destroy_trans(fa_trans_t *trans)
{
	int ret;

    ret = 0;

	if(trans) {
        if (trans->destroy_trans)
            ret = trans->destroy_trans(trans);

		if(!ret) {
			fa_free(trans);
			return 0;
		} else {
			return -1;
		}
		
	} else {
		return -1;
	}

    return 0;
}
