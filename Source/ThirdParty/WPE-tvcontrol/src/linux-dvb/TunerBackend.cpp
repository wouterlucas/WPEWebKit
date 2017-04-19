/*
 * Copyright (C) 2017 TATA ELXSI
 * Copyright (C) 2017 Metrological
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "TunerBackend.h"
#include "tv-log.h"

namespace LinuxDVB {

TvTunerBackend::TvTunerBackend(EventQueue<wpe_tvcontrol_event*>* eventQueue, int tunerCnt, std::unique_ptr<TunerData> tunerPtr)
    : m_tunerData(std::move(tunerPtr))
    , m_srcTypeListPtr(nullptr)
    , m_supportedSysCount(0)
    , m_eventQueue(eventQueue)
    , m_sType(Undifined)
{
    TvLogTrace();
    initializeSourceList();
    configureTuner(tunerCnt);
}

TvTunerBackend::~TvTunerBackend()
{
    TvLogTrace();
    m_tunerData = nullptr;
    if (m_srcTypeListPtr)
        delete m_srcTypeListPtr;
    clearSourceList();
    m_supportedSysCount = 0;
}

void TvTunerBackend::clearSourceList()
{
    while (!m_sourceList.empty()) {
        m_sourceList.pop_back();
    }
}

void TvTunerBackend::initializeSourceList()
{

    getAvailableSrcList(&m_srcList);
}

void TvTunerBackend::configureTuner(int tunerCnt)
{
    TvLogTrace();
    TvLogInfo("Getting Modulation for Tuner:%d", tunerCnt);

    /* Get configuration */
    getConfiguration();

    /* Get modulation from config  */
    char tmpStr[20];
    snprintf(tmpStr, 20, "TUNER_%d_MODULATION", tunerCnt + 1);
    std::string tunerStr = std::string(tmpStr);

    std::string modulation;
    if (m_configValues.find(tunerStr) != m_configValues.end()) {
        std::cout << "Modulation  found = " << m_configValues.find(tunerStr)->second << std::endl;
        modulation.assign(m_configValues.find(tunerStr)->second);
    } else {
        modulation.assign("8VSB");
        std::cout << "Setting default modulation\n";
    }
    std::cout << modulation << "\n";
    setModulation(modulation);
    populateFreq();
}

void TvTunerBackend::setModulation(std::string& modulation)
{
    struct dvbfe_handle* feHandle = openFE(m_tunerData->tunerId);
    if (feHandle) {
        /* Get tuner info */
        struct dvb_frontend_info feInfo;
        if (ioctl(feHandle->fd, FE_GET_INFO, &feInfo) == -1)
            TvLogInfo("   ERROR: unable to determine frontend type\n");

        /* Set modulation */
        if (!modulation.compare("8VSB")) {
            if (feHandle->type == DVBFE_TYPE_ATSC && (feInfo.caps & FE_CAN_8VSB)) {
                m_channel = ATSC_VSB; // FIXME:  check and remove

                /* Set the modulation */
                struct dtv_property p[] = { {.cmd = DTV_MODULATION } };
                struct dtv_properties cmdseq = {.num = 1, .props = p };
                struct dtv_property* propPtr;
                propPtr = p;
                propPtr->u.data = VSB_8;

                /* Set the current to platform*/
                if (ioctl(feHandle->fd, FE_SET_PROPERTY, &cmdseq) == -1)
                    TvLogInfo("Failed to set  Srource %d at plarform \n", ATSC_VSB);

                TvLogTrace();
                std::cout << "\nModulation set to 8VSB";
                propPtr->u.data = 0;
                m_tunerData->modulation = DVBFE_ATSC_MOD_VSB_8;
            } else // caps
                std::cout << "In sufficient capabilities details";
        } else // modulation
            std::cout << "In sufficient details";
        dvbfe_close(feHandle);
    } else
        TvLogInfo("Failed to open frontend \n");
}

void TvTunerBackend::populateFreq()
{
    for (int channel = 0; channel < 68; channel++) {
        if (baseOffset((channel + 2), m_channel) != -1)
            m_tunerData->frequency.push_back(baseOffset((channel + 2), m_channel) + ((channel + 2) * freqStep((channel + 2), m_channel)));
    }
}

void TvTunerBackend::getSignalStrength(double* signalStrength)
{
    TvLogTrace();
    struct dvbfe_handle* feHandle = openFE(m_tunerData->tunerId);
    if (feHandle) {
        struct dvbfe_info feInfo;
        ioctl(feHandle->fd, FE_READ_SIGNAL_STRENGTH, &feInfo.signal_strength);
        *signalStrength = static_cast<double>(10 * (log10(feInfo.signal_strength)) + 30);
        dvbfe_close(feHandle);
    } else
        TvLogInfo("Failed to open frontend \n");
}

/* modulation
 * return the base offsets for specified channellist and channel.
 */
int TvTunerBackend::baseOffset(int channel, int channelList)
{
    switch (channelList) {
    case ATSC_QAM: // ATSC cable, US EIA/NCTA Std Cable center freqs + IRC list
    case DVBC_BR: // BRAZIL - same range as ATSC IRCi
        switch (channel) {
        case 2 ... 4:
            return 45000000;
        case 5 ... 6:
            return 49000000;
        case 7 ... 13:
            return 135000000;
        case 14 ... 22:
            return 39000000;
        case 23 ... 94:
            return 81000000;
        case 95 ... 99:
            return -477000000;
        case 100 ... 133:
            return 51000000;
        default:
            return -1;
        }
    case ATSC_VSB: // ATSC terrestrial, US NTSC center freqs
        switch (channel) {
        case 2 ... 4:
            return 45000000;
        case 5 ... 6:
            return 49000000;
        case 7 ... 13:
            return 135000000;
        case 14 ... 69:
            return 389000000;
        default:
            return -1;
        }
    case ISDBT_6MHZ: // ISDB-T, 6 MHz central frequencies
        switch (channel) {
        // Channels 7-13 are reserved but aren't used yet
        // case  7 ... 13: return  135000000;
        case 14 ... 69:
            return 389000000;
        default:
            return -1;
        }
    case DVBT_AU: // AUSTRALIA, 7MHz step list
        switch (channel) {
        case 5 ... 12:
            return 142500000;
        case 21 ... 69:
            return 333500000;
        default:
            return -1;
        }
    case DVBT_DE: // GERMANY
    case DVBT_FR: // FRANCE, +/- offset 166kHz & +offset 332kHz & +offset 498kHz
    case DVBT_GB: // UNITED KINGDOM, +/- offset
        switch (channel) {
        case 5 ... 12:
            return 142500000; // VHF unused in FRANCE, skip those in offset loop
        case 21 ... 69:
            return 306000000;
        default:
            return -1;
        }

    case DVBC_QAM: // EUROPE
        switch (channel) {
        case 0 ... 1:
        case 5 ... 12:
            return 73000000;
        case 22 ... 90:
            return 138000000;
        default:
            return -1;
        }
    case DVBC_FI: // FINLAND, QAM128
        switch (channel) {
        case 1 ... 90:
            return 138000000;
        default:
            return -1;
        }
    case DVBC_FR: // FRANCE, needs user response.
        switch (channel) {
        case 1 ... 39:
            return 107000000;
        case 40 ... 89:
            return 138000000;
        default:
            return -1;
        }
    default:
        return -1;
    }
}

/*
 * return the freq step size for specified channellist
 */
int TvTunerBackend::freqStep(int channel, int channelList)
{
    switch (channelList) {
    case ATSC_QAM:
    case ATSC_VSB:
    case DVBC_BR:
    case ISDBT_6MHZ:
        return 6000000; // atsc, 6MHz step
    case DVBT_AU:
        return 7000000; // dvb-t australia, 7MHz step
    case DVBT_DE:
    case DVBT_FR:
    case DVBT_GB:
        switch (channel) { // dvb-t europe, 7MHz VHF ch5..12, all other 8MHz
        case 5 ... 12:
            return 7000000;
        case 21 ... 69:
            return 8000000;
        default:
            return 8000000; // should be never reached.
        }
    case DVBC_QAM:
    case DVBC_FI:
    case DVBC_FR:
        return 8000000; // dvb-c, 8MHz step
    default:
        return 0;
    }
}

tvcontrol_return TvTunerBackend::getSupportedSrcTypeList(wpe_tvcontrol_src_types_vector* outSourceTypesList)
{
    TvLogTrace();

    /* Get supported source list from platform*/
    if (getSupportedSourcesTypeList(outSourceTypesList) < 0) {
        TvLogInfo("Failed to get supported source list \n");
        return TVControlFailed;
    }
    return TVControlSuccess;
}

void TvTunerBackend::getAvailableSrcList(wpe_tvcontrol_src_types_vector* outSourceList)
{
    TvLogTrace();
    /* Get avaiable source list from platform*/
    if (getSupportedSourcesTypeList(outSourceList) < 0)
        TvLogInfo("Failed to get supported source list \n");
    /* Create private list of sources  */
    getSources();
}

void TvTunerBackend::getSources()
{
    TvLogTrace();

    if (m_srcTypeListPtr && m_supportedSysCount && m_sourceList.empty()) {
        /* Read supported type list from the private list
                       and create list of source objects */
        for (int i = 0; i < m_supportedSysCount; i++) {
            std::unique_ptr<SourceBackend> sInfo = std::make_unique<SourceBackend>(m_eventQueue, m_srcTypeListPtr[i], m_tunerData.get());
            m_sourceList.push_back(std::move(sInfo));
            TvLogTrace();
        }
    } else
        TvLogInfo("Private source list already created \n");
}

int TvTunerBackend::getSupportedSourcesTypeList(wpe_tvcontrol_src_types_vector* outSourceTypesList)
{
    TvLogInfo("Number of supported Source = \n ");

    if (!(m_srcTypeListPtr && m_supportedSysCount)) {
        TvLogTrace();
        struct dtv_property p = {.cmd = DTV_ENUM_DELSYS };
        struct dtv_properties cmdName = {.num = 1, .props = &p };
        struct dvbfe_handle* feHandle = openFE(m_tunerData->tunerId);
        if (feHandle) {
            TvLogTrace();
            if (ioctl(feHandle->fd, FE_GET_PROPERTY, &cmdName) == -1) {
                TvLogInfo("FE_GET_PROPERTY failed \n");
                dvbfe_close(feHandle);
                return -1;
            }

            m_supportedSysCount = cmdName.props->u.buffer.len;
            TvLogInfo("Number of supported Source = %" PRIu64 "\n", m_supportedSysCount);

            /* Create an array of  Type */
            m_srcTypeListPtr = (SourceType*)new SourceType[cmdName.props->u.buffer.len];
            TvLogTrace();

            for (int i = 0; i < m_supportedSysCount; i++) {
                /* Map the list  to W3C spec */
                switch (cmdName.props->u.buffer.data[i]) {
                case SYS_DVBC_ANNEX_A:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    // FIXME: m_srcTypeListPtr[i] = ;
                    break;
                case SYS_DVBC_ANNEX_B:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] = DvbC;
                    break;
                case SYS_DVBT:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] = DvbT;
                    break;
                case SYS_DSS:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    // FIXME: m_srcTypeListPtr[i] = ;
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
                    // FIXME: m_srcTypeListPtr[i] = ;
                    break;
                case SYS_DVBT2:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] = DvbT2;
                    break;
                case SYS_TURBO:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    // FIXME m_srcTypeListPtr[i] = ;
                    break;
                case SYS_DVBC_ANNEX_C:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    // FIXME m_srcTypeListPtr[i] = ;
                    break;
                case SYS_UNDEFINED:
                    TvLogInfo("STP: CASE = %d \t", cmdName.props->u.buffer.data[i]);
                    m_srcTypeListPtr[i] = Undifined;
                    break;
                default:
                    TvLogInfo("ST: DEFAULT  \n");
                    m_srcTypeListPtr[i] = Undifined;
                    break;
                } // switch
            } // Loop
            dvbfe_close(feHandle);
            if (!m_supportedSysCount) {
                outSourceTypesList->length = m_supportedSysCount;
                outSourceTypesList->types = nullptr;
                TvLogInfo("driver returned 0 supported delivery source type!");
                return -1;
            }
        } else {
            outSourceTypesList->length = m_supportedSysCount;
            outSourceTypesList->types = nullptr;
            TvLogInfo("Failed to open frontend \n");
            return -1;
        }

    } // List already created

    /* Update the number of supported Sources*/
    outSourceTypesList->length = m_supportedSysCount;
    /* Update source type ptr */
    outSourceTypesList->types = m_srcTypeListPtr;
    TvLogTrace();
    return m_supportedSysCount;
}

tvcontrol_return TvTunerBackend::startScanning(bool isRescanned)
{
    TvLogTrace();
    /* Get source corresponds to this type  */
    SourceBackend* source;
    getSource(m_sType, &source);
    return source->startScanning(isRescanned);
}

void TvTunerBackend::getSource(SourceType sType, SourceBackend** source)
{
    TvLogTrace();
    /* Iterate list and get the source matching the list*/
    for (auto& src : m_sourceList) {
        TvLogInfo("SRC type from list =  %d from param = %d", src->srcType(), sType);
        if (src->srcType() == sType)
            *source = src.get();
    }
}

tvcontrol_return TvTunerBackend::stopScanning()
{
    TvLogTrace();
    /* Get source corresponds to this type  */
    SourceBackend* source;
    getSource(m_sType, &source);
    return source->stopScanning();
}

tvcontrol_return TvTunerBackend::setCurrentChannel(SourceType sType, uint64_t channelNumber)
{
    TvLogTrace();
    /* Get source corresponds to this type  */
    SourceBackend* source;
    getSource(sType, &source);
    return source->setCurrentChannel(channelNumber);
}

tvcontrol_return TvTunerBackend::getChannels(SourceType sType, struct wpe_tvcontrol_channel_vector** channelVector)
{
    TvLogTrace();
    /* Get source corresponds to this type  */
    SourceBackend* source;
    getSource(sType, &source);
    return source->getChannels(channelVector);
}

tvcontrol_return TvTunerBackend::setCurrentSource(SourceType sType)
{
    tvcontrol_return ret = TVControlFailed;

    /* If sType different for already set source, change the src */
    if (getSrcType() != sType) {
        TvLogTrace();
       /* Set the current source to platform*/
        struct dvbfe_handle* feHandle = openFE(m_tunerData->tunerId);
        if (feHandle) {
            ret = TVControlSuccess;
            /* Retrive the source type corresponds to dvb*/
            struct dtv_property p[] = { {.cmd = DTV_DELIVERY_SYSTEM } };
            struct dtv_properties cmdseq = {.num = 1, .props = p };
            fe_delivery_system platSrcType;
            getSourceType(sType, &platSrcType);
            struct dtv_property* propPtr = p;
            propPtr->u.data = platSrcType;
            if (ioctl(feHandle->fd, FE_SET_PROPERTY, &cmdseq) == -1) {
                TvLogInfo("Failed to set  Srource %d at plarform \n ", platSrcType);
            } else {
                setSrcType(sType);
            }
            TvLogTrace();
            propPtr->u.data = SYS_UNDEFINED; // RESET
            /*Get the delivery system*/
            if (ioctl(feHandle->fd, FE_GET_PROPERTY, &cmdseq) == -1)
                TvLogInfo(" Get ioctl  failed \n");
            dvbfe_close(feHandle);
            TvLogInfo("Current v5 delivery system: %d \n", p[0].u.data);
        } else
            TvLogInfo("Failed to open frontend \n");
    }
    return ret;
}

void TvTunerBackend::getSourceType(SourceType sType, fe_delivery_system* platSrcType)
{
    TvLogTrace();
    switch (sType) {
#if 0
    case anexa: // FIXME
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
#if 0 // FIXME
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

void TvTunerBackend::getConfiguration()
{
    std::ifstream fileStream(CONFIGFILE);
    std::string line;
    while (std::getline(fileStream, line)) {
        std::istringstream isLine(line);
        std::string key;
        if (std::getline(isLine, key, '=')) {
            std::string value;
            if (key[0] == '#')
                continue;
            if (std::getline(isLine, value))
                m_configValues.insert(make_pair(key, value));
        }
    }
    if (m_configValues.empty()) {
        TvLogInfo("***************************************\n");
        TvLogInfo("******TVConfig file is MISSING*********\n");
    }
#ifdef TV_DEBUG
    TvLogInfo("Configuration details ");
    /* Iterate through all elements in std::map */
    for (auto& config : m_configValues)
        std::cout << config.first << " :: " << config.second << std::endl;
#endif
}

} // namespace LinuxDVB
