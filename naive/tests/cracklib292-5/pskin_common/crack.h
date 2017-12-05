#ifndef CRACKLIB_H
#define CRACKLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/* Pass these functions a password (pw) and a path to the
 * dictionaries (/usr/lib/cracklib_dict should be specified)
 * and it will either return a NULL string, meaning that the
 * password is good, or a pointer to a string that explains the
 * problem with the password.
 *
 * FascistCheckUser() executes tests against an arbitrary user (the 'gecos'
 * attribute can be NULL), while FascistCheck() assumes the currently logged
 * in user.
 *
 * You must link with -lcrack
 */

extern const char *FascistCheck(const char *pw, const char *dictpath);
extern const char *FascistCheckUser(const char *pw, const char *dictpath,
				    const char *user, const char *gecos);

/* This Debian specific method is a work-around for Debian #682735. Please
   do not rely on it being available in future verisons of cracklib2.
   Returns 1 (true) for success and 0 (false) in case an error occurred
   opening or reading the dictionary. In the later case, please check
   errno. */
extern int __DEBIAN_SPECIFIC__SafeFascistCheck(const char *pw,
					const char *dictpath, char *errmsg,
                                        size_t errmsg_len);

/* This function returns the compiled in value for DEFAULT_CRACKLIB_DICT.
 */
extern const char *GetDefaultCracklibDict(void);

#ifdef __cplusplus
};
#endif

#endif
