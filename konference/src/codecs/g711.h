#ifndef G711_H_
#define G711_H_

#include <qsocketdevice.h>
#include "codecbase.h"


extern uchar alaw_comp_table[16384];
extern uchar ulaw_comp_table[16384];
extern short alaw_exp_table[256];
extern short ulaw_exp_table[256];

class g711alaw : public codecBase
{
public:
	g711alaw() :codecBase() {};
	~g711alaw() {};
	virtual int Decode(uchar *In, short *out, int Len, short &maxPower);
	virtual int Encode(short *In, uchar *out, int Samples, short &maxPower, int gain);
	virtual int Silence(uchar *out, int ms);
	virtual QString getCodecName() { return "G711a"; };
	virtual int getPayload(){ return 0x08; };
private:
};

class g711ulaw : public codecBase
{
public:
	g711ulaw() :codecBase() {};
	~g711ulaw() {};
	virtual int Decode(uchar *In, short *out, int Len, short &maxPower);
	virtual int Encode(short *In, uchar *out, int Samples, short &maxPower, int gain);
	virtual int Silence(uchar *out, int ms);
	virtual QString getCodecName() { return "G711u"; };
	virtual int getPayload(){ return 0x00; };
private:
};

#endif
