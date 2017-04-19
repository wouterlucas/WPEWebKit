#ifndef TUNER_BACKEND_H_
#define TUNER_BACKEND_H_

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <unistd.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/dvb/frontend.h>
#include <stdint.h>
#include <libdvbapi/dvbfe.h>
#include "TVConfig.h"

using namespace std;

#define FE_STATUS_PARAMS (DVBFE_INFO_LOCKSTATUS|DVBFE_INFO_FEPARAMS|DVBFE_INFO_SIGNAL_STRENGTH|DVBFE_INFO_BER|DVBFE_INFO_SNR|DVBFE_INFO_UNCORRECTED_BLOCKS)

struct dvbfe_handle {
    int fd;
    enum dvbfe_type type;
    char *name;
};

namespace BCMRPi {

class TvTunerBackend {
public:
   virtual ~TvTunerBackend() {}
   void getTunerInfo();
   void getModulation();
   void populateFreq(ChannelList);

private:
   TvTunerBackend() {}

   ChannelList m_channel;
   struct dvb_frontend_info m_feInfo;
   struct dvbfe_handle* m_feHandle;
   struct dvbfe_info fe_info;

   int baseOffset(int channel, int channelList);
   int freqStep(int channel, int channelList);
};

} // namespace BCMRPi
#endif //TUNER_BACKEND_H_
