// careful enabling this!
#define ENABLE_LFS 1

#if defined (ENABLE_LFS)
	#define ftell				ftello64
	#define fseek				fseeko64
	#define fopen				fopen64
	#define off_t				off64_t
#else
	#define off_t				long int
#endif
