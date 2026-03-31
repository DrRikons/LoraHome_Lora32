#ifndef _ELOG_CFG_H_
#define _ELOG_CFG_H_

/* enable log output. */
#define ELOG_OUTPUT_ENABLE
/* setting static output log level. range: from ELOG_LVL_ASSERT to ELOG_LVL_VERBOSE */
#define ELOG_OUTPUT_LVL                          ELOG_LVL_VERBOSE
/* enable assert check */
#define ELOG_ASSERT_ENABLE
/* buffer size for every line's log */
#define ELOG_LINE_BUF_SIZE                       512
/* output line number max length */
#define ELOG_LINE_NUM_MAX_LEN                    5
/* output filter's tag max length */
#define ELOG_FILTER_TAG_MAX_LEN                  16
/* output filter's keyword max length */
#define ELOG_FILTER_KW_MAX_LEN                   16
/* output newline sign */
#define ELOG_NEWLINE_SIGN                        "\n"

#endif /* _ELOG_CFG_H_ */