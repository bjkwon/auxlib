#include <exception>
#include <vector>
#include <queue>
#include "sigproc.h"
#include "AUXLib.h"

#define MAX_AUX_ERR_MSG_LEN	1000

vector<CAstSig *> GAstSigs;
vector<CSignals> GSigs;
queue<int> GAstSigRecycle;
char GAstErrMsg[MAX_AUX_ERR_MSG_LEN] = "";

int AUXNew(const int sample_rate, const char *wavepath, const char *udfpath, const char *datapath) // Returns new handle, which is 0 or positive, or -1 on error.
{
	int handle;
	try {
		CAstSig *pAstSig = (sample_rate > 1) ? new CAstSig(sample_rate) : new CAstSig();
		if (wavepath)
			pAstSig->SetPath("wav", wavepath);
		if (udfpath)
			pAstSig->SetPath("aux", udfpath);
		if (datapath)
			pAstSig->SetPath("txt", datapath);
		if (GAstSigRecycle.empty()) {
			handle = (int)GAstSigs.size();
			GAstSigs.push_back(pAstSig);
			GSigs.push_back(CSignals());
		} else {
			handle = GAstSigRecycle.front();
			GAstSigRecycle.pop();
			GAstSigs[handle] = pAstSig;
		}
	} catch (const exception &e) {
		strncpy(GAstErrMsg, e.what(), MAX_AUX_ERR_MSG_LEN);
		return -1;
	}
	return handle;
}


void AUXDelete(const int hAUX) // Ignores invalid hAUX
{
	if (hAUX<0 || hAUX>(int)GAstSigs.size()-1)
		return;
	if (CAstSig *pAstSig = GAstSigs[hAUX]) {
		delete pAstSig;
		GAstSigs[hAUX] = NULL;
		GAstSigRecycle.push(hAUX);
	}
}


int AUXEval(const int hAUX, const char *strIn, double **buffer, int *length) // Returns the number of channels or
//  0 : Empty result
// -1 : Invalid hAUX
// -2 : AUX error
// -3 : Unknown error
//
// Actual size of buffer is (length * channels).
// Data in buffer are valid until next AUXEval() or AUXDelete() with the same hAUX.
{
	try {
		if (hAUX<0 || hAUX>(int)GAstSigs.size()-1) {
			strncpy(GAstErrMsg, "AUXLib error: Invalid handle.", MAX_AUX_ERR_MSG_LEN);
			return -1;
		}
		CAstSig *pAstSig = GAstSigs[hAUX];
		if (!pAstSig) {
			strncpy(GAstErrMsg, "AUXLib error: Invalid handle - already deleted.", MAX_AUX_ERR_MSG_LEN);
			return -1;
		}
		int nChannel;
		GSigs[hAUX] = pAstSig->SetNewScript(strIn).Compute();
		GSigs[hAUX].MakeChainless();
		if (length)
			*length = GSigs[hAUX].GetLength();
		if (length && *length)
			nChannel = 1;
		else
			return 0;
		if (buffer) {
			if (CSignals *psig = GSigs[hAUX].DetachNextChan()) {
				for (CSignals *p=psig; p; p=p->GetNextChan(),nChannel++)
					GSigs[hAUX] += p;	// Now that Sig is mono (detached), += concatenates only the first channel of RHS
				delete psig;
				GSigs[hAUX].MakeChainless();
			}
			*buffer = GSigs[hAUX].buf;
		}
		return nChannel;
	} catch (const char *errmsg) {
		strncpy(GAstErrMsg, errmsg, MAX_AUX_ERR_MSG_LEN);
		return -2;
	} catch (exception &e) {
		strncpy(GAstErrMsg, e.what(), MAX_AUX_ERR_MSG_LEN);
		return -3;
	}
}


int AUXPlay(const int hAUX, const int DevID)
{
	char errstr[250];
	if (hAUX<0 || hAUX>(int)GAstSigs.size()-1) {
		strncpy(GAstErrMsg, "AUXLib error: Invalid handle.", MAX_AUX_ERR_MSG_LEN);
		return 0;
	}
	CAstSig *pAstSig = GAstSigs[hAUX];
	if (!pAstSig) {
		strncpy(GAstErrMsg, "AUXLib error: Invalid handle - already deleted.", MAX_AUX_ERR_MSG_LEN);
		return 0;
	}
	if (pAstSig->Sig.PlayArray(DevID, errstr)) {
		strncpy(GAstErrMsg, errstr, MAX_AUX_ERR_MSG_LEN);
		return 0;
	} else
		return 1;
}


int AUXWavwrite(const int hAUX, const char *filename)
{
	char errstr[250];
	if (hAUX<0 || hAUX>(int)GAstSigs.size()-1) {
		strncpy(GAstErrMsg, "AUXLib error: Invalid handle.", MAX_AUX_ERR_MSG_LEN);
		return 0;
	}
	CAstSig *pAstSig = GAstSigs[hAUX];
	if (!pAstSig) {
		strncpy(GAstErrMsg, "AUXLib error: Invalid handle - already deleted.", MAX_AUX_ERR_MSG_LEN);
		return 0;
	}
	if (!pAstSig->Sig.Wavwrite(filename, errstr)) {
		strncpy(GAstErrMsg, errstr, MAX_AUX_ERR_MSG_LEN);
		return 0;
	} else
		return 1;
}


const char *AUXGetErrMsg(void) // Returns error message for the last AUX error.
{
	GAstErrMsg[MAX_AUX_ERR_MSG_LEN-1] = '\0';
	return GAstErrMsg;
}


int AUXGetInfo(const int hAUX, const char *name, void *output)
{
	if (hAUX<0 || hAUX>(int)GAstSigs.size()-1) {
		strncpy(GAstErrMsg, "AUXLib error: Invalid handle.", MAX_AUX_ERR_MSG_LEN);
		return 0;
	}
	CAstSig *pAstSig = GAstSigs[hAUX];
	if (!pAstSig) {
		strncpy(GAstErrMsg, "AUXLib error: Invalid handle - already deleted.", MAX_AUX_ERR_MSG_LEN);
		return 0;
	}
	if (strcmp(name, "wavepath") == 0) {
		strcpy((char *)output, pAstSig->GetPath("wav"));
	} else if (strcmp(name, "auxpath") == 0) {
		strcpy((char *)output, pAstSig->GetPath("aux"));
	} else if (strcmp(name, "datapath") == 0) {
		strcpy((char *)output, pAstSig->GetPath("txt"));
	} else if (strcmp(name, "fs") == 0) {
		int fs = pAstSig->GetFs();
		memcpy(output, &fs, sizeof(int));
	} else {
		strncpy(GAstErrMsg, "AUXLib error: Invalid property name was passed to AUXGetInfo().", MAX_AUX_ERR_MSG_LEN);
		return 0;
	}
	return 1;
}