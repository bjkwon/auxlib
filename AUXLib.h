__declspec (dllexport) int AUXNew(const int sample_rate, const char *wavepath, const char *udfpath, const char *datapath);
__declspec (dllexport) void AUXDelete(const int hAUX);
__declspec (dllexport) int AUXEval(const int hAUX, const char *strIn, double **buffer, int *length);
__declspec (dllexport) int AUXPlay(const int hAUX, const int DevID);
__declspec (dllexport) int AUXWavwrite(const int hAUX, const char *filename);
__declspec (dllexport) const char *AUXGetErrMsg(void);
__declspec (dllexport) int AUXGetInfo(const int hAUX, const char *name, void *output);
