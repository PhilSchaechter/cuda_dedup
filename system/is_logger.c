/*
 * is_logger.c
 *
 *  Created on: Nov 8, 2018
 *      Author: phil
 */

#include <zlog.h>
#include <assert.h>

zlog_category_t *logger = NULL;

zlog_category_t *is_init_logging() {
int rc = zlog_init("../system/zlog/zlog_dev.conf");
assert(!rc);
logger = zlog_get_category("is");

return logger;
}
