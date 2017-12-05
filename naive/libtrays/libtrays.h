#ifndef	_LIBTRAYS_H
#define	_LIBTRAYS_H

/* 
 * This is the set you want to implement.
 * 
 * trays_init called early on in main
 * trays_log called at each basic block
 * trays_log_function_entry called as labeled
 * trays_log_function_exit called as labeled
 * trays_log_pc called before compares
 *
 */
void	trays_init(const char *fp);
void	trays_log(const char *msg, uint64_t ba);
void	trays_log_function_entry(const char *msg, uint64_t ba);
void	trays_log_function_exit(const char *msg, uint64_t ba);
void	trays_log_pc();

#endif
