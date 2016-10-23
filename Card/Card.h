// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the CARD_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// CARD_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef CARD_EXPORTS
#define CARD_API __declspec(dllexport)
#else
#define CARD_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

CARD_API bool GetCard(int index, int *color, int *number);

CARD_API bool IsPlus4(int index);

CARD_API bool IsSame(int _1, int _2);

#ifdef __cplusplus
};
#endif
