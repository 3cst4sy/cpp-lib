#ifndef CPP_LIB_WINDOWS_ISO8601
#define CPP_LIB_WINDOWS_ISO8601

#ifndef WIN32
  #error "Sorry: The feature requested is only available on Windows."
#endif

#include <time.h>

bool parseISO8601(char *text, tm& tmstruct, char& flag);

#endif // CPP_LIB_WINDOWS_ISO8601
