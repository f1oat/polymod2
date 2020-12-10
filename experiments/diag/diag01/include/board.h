#if defined(ARDUINO_ARCH_MEGAAVR)
#define F(str) (str)
#define NO_FLASH_STRING
# define cli()  __asm__ __volatile__ ("cli" ::: "memory")
#define Serial Serial1
#define ATMEGA_4809
#endif

