#include "TunerBackend.h"
#include "tv-log.h"

namespace BCMRPi {

TvTunerBackend::TvTunerBackend(EventQueue<wpe_tvcontrol_event*>* eventQueue, int tunerCnt, TunerData* tunerPtr)
    : m_tunerData(tunerPtr)
    , m_srcTypeListPtr(NULL)
    , m_supportedSysCount(0)
    , m_eventQueue(eventQueue)
    , m_sType(Undifined) {

    TvLogTrace();
    initializeSourceList();
    configureTuner(tunerCnt);
}

TvTunerBackend::~TvTunerBackend() {
    TvLogTrace();
    if (m_tunerData)
        delete m_tunerData;
    m_tunerData = nullptr;

    m_supportedSysCount = 0;
}

void TvTunerBackend::initializeSourceList() {

    getAvailableSrcList(&m_srcList);
}

void TvTunerBackend::configureTuner(int tunerCnt) {
    TvLogTrace();
    string  data, tunerStr;
    char tmpStr[20];
    std::string modulation;
    TvLogInfo("Getting Modulation for Tuner:%d", tunerCnt);

    /* Get configuration */
    getConfiguration();

    /* Get modulation from config  */
    snprintf(tmpStr, 20, "TUNER_%d_MODULATION", tunerCnt + 1);
    tunerStr = string(tmpStr);
    if (m_configValues.find(tunerStr) != m_configValues.end()) {
        std::cout << "Modulation  found = " << m_configValues.find(tunerStr)->second << std::endl;
        modulation.assign(m_configValues.find(tunerStr)->second) ;
    } else {
        modulation.assign("8VSB");
        std::cout << "Setting default modulation\n";
    }
    std::cout << modulation << "\n";
    setModulation(modulation);
    populateFreq();
}

void TvTunerBackend::setModulation(std::string& modulation) {

    struct dvb_frontend_info feInfo;
    struct dvbfe_handle* feHandle = openFE(m_tunerData->tunerId);
    if (feHandle) {
        /* Get tuner info */
        if (ioctl(feHandle->fd, FE_GET_INFO, &feInfo) == -1) {
            TvLogInfo("   ERROR: unable to determine frontend type\n");
        }

        /* Set modulation */
        if (!modulation.compare("8VSB")) { //TODO add other modulations
            if (feHandle->type ==  DVBFE_TYPE_ATSC && (feInfo.caps & FE_CAN_8VSB)) {
                m_channel = ATSC_VSB; //TODO  check and remove

                /* Set the modulation */
                struct dtv_property p[] = {{.cmd = DTV_MODULATION}};
                struct dtv_properties cmdseq = {.num = 1, .props = p};
                struct dtv_property *propPtr;
                propPtr = p;
                propPtr->u.data = VSB_8;

                /* Set the current to platform*/
                if (ioctl(feHandle->fd, FE_SET_PROPERTY, &cmdseq) == -1) {
                    TvLogInfo("Failed to set  Srource %d at plarform \n",ATSC_VSB);
                }
                TvLogTrace();
                cout << "\nModulation set to 8VSB";
                propPtr->u.data = 0;

                m_tunerData->modulation = DVBFE_ATSC_MOD_VSB_8;
            } else { //caps
                cout << "In sufficient capabilities details";
            }
        } else { //modulation
            cout << "In sufficient details";
        }
        dvbfe_close(feHandle);
    } else {
        TvLogInfo("Failed to open frontend \n");
    }
}

void TvTunerBackend::populateFreq() {
    for (int channel = 0; channel < 68 ;channel++) {
        if (baseOffset(channel+2, m_channel) != -1)
            m_tunerData->frequency.push_back(baseOffset( channel+2, m_channel) + ((channel+2) * freqStep(channel+2, m_channel)));
    }
}

void TvTunerBackend::getSignalStrength(double* signalStrength) {
    struct dvbfe_handle* feHandle = openFE(m_tunerData->tunerId);

    if (feHandle) {
        struct dvbfe_info fe_info;
        TvLogTrace();
        ioctl(feHandle->fd, FE_READ_SIGNAL_STRENGTH, fe_info.signal_strength);
        *signalStrength = static_cast<double>(10 *(log10(fe_info.signal_strength)) + 30);
        dvbfe_close(feHandle);
    } else {
        TvLogInfo("Failed to open frontend \n");
    }
}

/*modulation
 * return the base offsets for specified channellist and channel.
 */
int TvTunerBackend::baseOffset(int channel, int channelList) {
    switch (channelList) {
    case ATSC_QAM: //ATSC cable, US EIA/NCTA Std Cable center freqs + IRC list
    case DVBC_BR:  //BRAZIL - same range as ATSC IRCi
        switch (channel) {
        case  2 ...  4:
            return   45000000;
        case  5 ...  6:
            return   49000000;
        case  7 ... 13:
            return  135000000;
        case 14 ... 22:
            return   39000000;
        case 23 ... 94:
            return   81000000;
        case 95 ... 99:
            return -477000000;
        case 100 ... 133:
            return   51000000;
        default:
            return   -1;
        }
    case ATSC_VSB: //ATSC terrestrial, US NTSC center freqs
        switch (channel) {
        case  2 ...  4:
            return   45000000;
        case  5 ...  6:
            return   49000000;
        case  7 ... 13:
            return  135000000;
        case 14 ... 69:
            return  389000000;
        default:
            return  -1;
        }
    case ISDBT_6MHZ: // ISDB-T, 6 MHz central frequencies
        switch (channel) {
            // Channels 7-13 are reserved but aren't used yet
            //case  7 ... 13: return  135000000;
        case 14 ... 69:
            return  389000000;
        default:
            return  -1;
        }
    case DVBT_AU://AUSTRALIA, 7MHz step list
        switch (channel) {
        case  5 ... 12:
            return  142500000;
        case 21 ... 69:
            return  333500000;
        default:
            return  -1;
        }
    case DVBT_DE://GERMANY
    case DVBT_FR://FRANCE, +/- offset 166kHz & +offset 332kHz & +offset 498kHz
    case DVBT_GB://UNITED KINGDOM, +/- offset
        switch (channel) {
        case  5 ... 12:
            return  142500000; // VHF unused in FRANCE, skip those in offset loop
        case 21 ... 69:
            return  306000000;
        default:
            return  -1;
        }

    case DVBC_QAM: //EUROPE
        switch (channel) {
        case  0 ... 1:
        case  5 ... 12:
            return   73000000;
        case 22 ... 90:
            return  138000000;
        default:
            return  -1;
        }
    case DVBC_FI:  //FINLAND, QAM128
        switch (channel) {
        case  1 ... 90:
            return  138000000;
        default:
            return  -1;
        }
    case DVBC_FR:  //FRANCE, needs user response.
        switch (channel) {
        case  1 ... 39:
            return  107000000;
        case 40 ... 89:
            return  138000000;
        default:
            return  -1;
        }
     default:
         return -1;
    }
}

/*
 * return the freq step size for specified channellist
 */
int TvTunerBackend::freqStep(int channel, int channelList) {
    switch (channelList) {
    case ATSC_QAM:
    case ATSC_VSB:
    case DVBC_BR:
    case ISDBT_6MHZ:
        return  6000000; // atsc, 6MHz step
    case DVBT_AU:
        return  7000000; // dvb-t australia, 7MHz step
    case DVBT_DE:
    case DVBT_FR:
    case DVBT_GB:
        switch (channel) { // dvb-t europe, 7MHz VHF ch5..12, all other 8MHz
        case  5 ... 12:
            return 7000000;
        case 21 ... 69:
            return 8000000;
        default:
            return 8000000; // should be never reached.
        }
    case DVBC_QAM:
    case DVBC_FI:
    case DVBC_FR:
        return  8000000; // dvb-c, 8MHz step
    default:
        return 0;
    }
}

tvcontrol_return TvTunerBackend::getSupportedSrcTypeList(wpe_tvcontrol_src_types_vector* out_source_types_list) {
    TvLogTrace();

    /* Get supported source list from platform*/
    int  ret = 0;
    ret  =  getSupportedSourcesTypeList(out_source_types_list);
    if (ret < 0) {
        TvLogInfo("Failed to get supported source list \n");
        return TVControlFailed;
    }
    return TVControlSuccess;
}

void TvTunerBackend::getAvailableSrcList(wpe_tvcontrol_src_types_vector* out_source_list) {
    TvLogTrace();
    /* Get avaiable source list from platform*/
    int  ret = 0;
    ret  =  getSupportedSourcesTypeList(out_source_list);
    if (ret < 0)
        TvLogInfo("Failed to get supported source list \n");
    /* Create private list of sources  */
    getSources();
}

void TvTunerBackend::getSources() {
    TvLogTrace();
    int i;

    if (m_srcTypeListPtr && (m_supportedSysCount != 0) && m_sourceList.empty()) {
        /* Read supported type list from the private list
                       and create list of source objects */
        for (i = 0; i < m_supportedSysCount; i++) {
            SourceBackend* sInfo = (SourceBackend* )new SourceBackend(m_eventQueue, m_srcTypeListPtr[i], m_tunerData);
            m_sourceList.push_back(sInfo);
            TvLogTrace();
        }
    } else {
        TvLogInfo("Private source list already created \n");
    }
}

int TvTunerBackend::getSupportedSourcesTypeList(wpe_tvcontrol_src_types_vector* out_source_types_list) {

    int i = 0;
    struct dvbfe_handle* feHandle;
    TvLogInfo("Number of supported Source = \n ");

    if (!m_srcTypeListPtr && (m_supportedSysCount == 0)) {
        TvLogTrace();
        struct dtv_property p = {.cmd = DTV_ENUM_DELSYS };
        struct dtv_properties cmdName = {.num = 1, .props = &p};
        feHandle = openFE(m_tunerData->tunerId);
        if (feHandle) {
            TvLogTrace();
            if (ioctl(feHandle->fd, FE_GET_PROPERTY, &cmdName) == -1) {
                TvLogInfo("FE_GET_PROPERTY failed \n");
                dvbfe_close(feHandle);
                return -1;
            }

            m_supportedSysCount = cmdName.props->u.buffer.len;
            TvLogInfo("Number of supported Source = %"  PRIu64 "\n", m_supportedSysCount);

            /*Create an array of  Type */
            m_srcTypeListPtr = (SourceType *)new SourceType[cmdName.props->u.buffer.len];
            TvLogTrace();

            for (i = 0; i < m_supportedSysCount; i++) {
                /*Map the list  to W3C spec */
                switch (cmdName.props->u.buffer.data[i]) {
                case SYS_DVBC_ANNEX_A:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    //TODO m_srcTypeListPtr[i] = ;
                    break;
                case SYS_DVBC_ANNEX_B:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] =  DvbC; //TODO
                    break;
                case SYS_DVBT:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] = DvbT;
                    break;
                case SYS_DSS:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    //TODO m_srcTypeListPtr[i] = ;
                    break;
                case SYS_DVBS:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] = DvbS;
                    break;
                case SYS_DVBS2:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] = DvbS2;
                    break;
                case SYS_DVBH:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] = DvbH;
                    break;
                case SYS_ISDBT:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] = IsdbT;
                    break;
                case SYS_ISDBS:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] = IsdbS;
                    break;
                case SYS_ISDBC:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] = IsdbC;
                    break;
                case SYS_ATSC:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] = Atsc;
                    break;
                case SYS_ATSCMH:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] = AtscMH;
                    break;
                case SYS_DTMB:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] = Dtmb;
                    break;
                case SYS_CMMB:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] = Cmmb;
                    break;
                case SYS_DAB:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    //TODO m_srcTypeListPtr[i] = ;
                    break;
                case SYS_DVBT2:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] = DvbT2;
                    break;
                case SYS_TURBO:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                   //TODO m_srcTypeListPtr[i] = ;
                    break;
                case SYS_DVBC_ANNEX_C:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    //TODO m_srcTypeListPtr[i] = ;
                    break;
                case SYS_UNDEFINED:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] = Undifined;
                    break;
                default:
                    TvLogInfo("ST: DEFAULT  \n");
                    m_srcTypeListPtr[i] = Undifined;
                    break;
                } //switch
            } // Loop
            dvbfe_close(feHandle);
            if (m_supportedSysCount == 0) {
                out_source_types_list->length = m_supportedSysCount;
                out_source_types_list->types = NULL;
                TvLogInfo("driver returned 0 supported delivery source type!");
                return -1;
            }
        } else {
            out_source_types_list->length = m_supportedSysCount;
            out_source_types_list->types = NULL;
            TvLogInfo("Failed to open frontend \n");
            return -1;
        }

    } //List already created

    /*Update the number of supported Sources*/
    out_source_types_list->length = m_supportedSysCount;
    TvLogTrace();
    /* update source type  ptr */
    out_source_types_list->types = m_srcTypeListPtr;
    TvLogTrace();
    return m_supportedSysCount;
}

tvcontrol_return TvTunerBackend::startScanning(bool isRescanned) {
    tvcontrol_return ret = TVControlFailed;
    TvLogTrace();
    /* Get source corresponds to this type  */
    SourceBackend *source;
    getSource(m_sType, &source);
    ret = source->startScanning(isRescanned);
    return ret;
}

void TvTunerBackend::getSource(SourceType sType, SourceBackend **source) {
    TvLogTrace();
    /* Iterate list and get the source matching the list*/
    for (auto& src : m_sourceList) {
        TvLogInfo("SRC type from list =  %d from param = %d", src->srcType(), sType);
        if (src->srcType() == sType) {
            *source = src;
        }
    }
}

tvcontrol_return TvTunerBackend::stopScanning() {
    tvcontrol_return ret = TVControlFailed;
    TvLogTrace();
    /* Get source corresponds to this type  */
    SourceBackend *source;
    getSource(m_sType, &source);
    ret = source->stopScanning();
    return ret;
}

tvcontrol_return TvTunerBackend::setCurrentChannel(SourceType sType ,uint64_t channelNumber) {
    tvcontrol_return ret = TVControlFailed;
    TvLogTrace();
    /* Get source corresponds to this type  */
    SourceBackend *source;
    getSource(sType, &source);
    ret = source->setCurrentChannel(channelNumber);
    return ret;
}

tvcontrol_return TvTunerBackend::getChannels(SourceType sType, struct wpe_tvcontrol_channel_vector** channelVector) {
    tvcontrol_return ret = TVControlFailed;
    printf("%s:%s:%d \n", __FILE__, __func__, __LINE__);
    /* Get source corresponds to this type  */
    SourceBackend *source;
    getSource(sType, &source);
    ret = source->getChannels(channelVector);
    return ret;
}

tvcontrol_return TvTunerBackend::setCurrentSource(SourceType sType) {
    tvcontrol_return ret = TVControlFailed;
    fe_delivery_system  platSrcType;
    struct dtv_property p[] = {{.cmd = DTV_DELIVERY_SYSTEM}};
    struct dtv_properties cmdseq = {.num = 1, .props = p};
    struct dtv_property *propPtr;
    struct dvbfe_handle* feHandle;

    /* If sType different for already set source, change the src */
    if (getSrcType() != sType) {
        TvLogTrace();
        /* Retrive the source type corresponds to dvb*/
        getSourceType(sType, &platSrcType);
        propPtr = p;
        propPtr->u.data = platSrcType;
        /* Set the current source to platform*/
        feHandle = openFE(m_tunerData->tunerId);
        if (feHandle) {
            ret = TVControlSuccess;
            if (ioctl(feHandle->fd, FE_SET_PROPERTY, &cmdseq) == -1) {
                TvLogInfo("Failed to set  Srource %d at plarform \n ",platSrcType);
            } else {
                /* Set the value of private member */
                setSrcType(sType);
                /* Sent on source changed */
            }
            TvLogTrace();
            propPtr->u.data = SYS_UNDEFINED; //RESET

            /*Get the delivery system*/
            if (ioctl(feHandle->fd, FE_GET_PROPERTY, &cmdseq) == -1) {
                TvLogInfo(" Get ioctl  failed \n");
            }
            dvbfe_close(feHandle);
            TvLogInfo("Current v5 delivery system: %d \n", p[0].u.data);
        } else {
            TvLogInfo("Failed to open frontend \n");
        }
    }
    return ret;
}

void TvTunerBackend::getSourceType(SourceType sType ,fe_delivery_system* platSrcType) {
    TvLogTrace();
    switch (sType) {
#if 0
    case anexa: //TODO
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_DVBC_ANNEX_B;
        break;
#endif
    case DvbC:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_DVBC_ANNEX_B;
        break;
    case DvbT:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_DVBT;
        break;
#if 0 //TODO
    case dss:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_DSS;
        break;
#endif
    case DvbS:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_DVBS;
        break;
    case DvbS2:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_DVBS2;
        break;
    case DvbH:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_DVBH;
        break;
    case IsdbT:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_ISDBT;
        break;
    case IsdbS:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_ISDBS;
        break;
    case IsdbC:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_ISDBC;
        break;
    case Atsc:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_ATSC;
        break;
    case AtscMH:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_ATSCMH;
        break;
    case Dtmb:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_DTMB;
        break;
    case Cmmb:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_CMMB;
        break;
#if 0
    case dab:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_DAB;
        break;
#endif
    case DvbT2:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_DVBT2;
        break;
#if 0
    case sturbo:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_TURBO;
        break;
    case annexc:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_DVBC_ANNEX_C;
        break;
#endif
    case Undifined:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_UNDEFINED;
        break;
    default:
        TvLogInfo("STP: CASE = %d \t", sType);
        *platSrcType = SYS_UNDEFINED;
         break;
    }
    TvLogTrace();
}

void TvTunerBackend::getConfiguration() {

    std::ifstream fileStream(CONFIGFILE);
    std::string line;
    while (std::getline(fileStream, line)) {
        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key,'=')) {
            std::string value;
            if (key[0] == '#')
                continue;
            if (std::getline(is_line, value)) {
                m_configValues.insert(make_pair(key,value));
            }
        }
    }
    if (m_configValues.empty()) {
        TvLogInfo("***************************************\n");
        TvLogInfo("******TVConfig file is MISSING*********\n");
    }
#ifdef TV_DEBUG
    std::cout << "Configuration details " << "\n" ;
    /* Iterate through all elements in std::map */
    ConfigInfo::const_iterator::iterator it = m_configValues.begin();
    while (it != m_configValues.end()) {
        std::cout<<it->first<<" :: "<<it->second<<std::endl;
        it++;
    }
#endif

}

} // namespace BCMRPi
