// rwx user
#define IS_R_USR(m)      (((m) & S_IRUSR) == S_IRUSR)     // is write user
#define IS_W_USR(m)      (((m) & S_IWUSR) == S_IWUSR)     // is read user
#define IS_X_USR(m)      (((m) & S_IXUSR) == S_IXUSR)     // is execute user

// rwx group
#define IS_R_GRP(m)      (((m) & S_IRGRP) == S_IRGRP)     // is write group
#define IS_W_GRP(m)      (((m) & S_IWGRP) == S_IWGRP)     // is read group
#define IS_X_GRP(m)      (((m) & S_IXGRP) == S_IXGRP)     // is execute group

// rwx other
#define IS_R_OTH(m)      (((m) & S_IROTH) == S_IROTH)     // is write other
#define IS_W_OTH(m)      (((m) & S_IWOTH) == S_IWOTH)     // is read other
#define IS_X_OTH(m)      (((m) & S_IXOTH) == S_IXOTH)     // is execute other

// String
#define IS_EQUAL(a,b)   (strcmp(a, b) == 0)
#define IS_FOLDER_POINTER(dirname)   (IS_EQUAL(dirname, ".") || IS_EQUAL(dirname, ".."))

// check if it is the same inode on the same device
#define SAME_INODE(a, b) ((a).st_ino == (b).st_ino && (a).st_dev == (b).st_dev)


#define TYPE_SIGNED(t) (! ((t) 0 < (t) -1))

/* Minimum and maximum values for integer types and expressions.  */

/* The width in bits of the integer type or expression T.
   Do not evaluate T.
   Padding bits are not supported; this is checked at compile-time below.  */
#define TYPE_WIDTH(t) (sizeof (t) * CHAR_BIT)

/* The maximum and minimum values for the integer type T.  */
#define TYPE_MINIMUM(t) ((t) ~ TYPE_MAXIMUM (t))
#define TYPE_MAXIMUM(t)                                                 \
  ((t) (! TYPE_SIGNED (t)                                               \
        ? (t) -1                                                        \
        : ((((t) 1 << (TYPE_WIDTH (t) - 2)) - 1) * 2 + 1)))

#ifndef OFF_T_MIN
# define OFF_T_MIN TYPE_MINIMUM (off_t)
#endif

#ifndef OFF_T_MAX
# define OFF_T_MAX TYPE_MAXIMUM (off_t)
#endif

/*
static uintmax_t
unsigned_file_size (off_t size)
{
    return size + (size < 0) * ((uintmax_t) OFF_T_MAX - OFF_T_MIN + 1);
}
*/