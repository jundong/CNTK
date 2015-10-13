//
// <copyright file="UCIFastReader.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// UCIFastReader.h - Include file for the MTK and MLF format of features and samples 
#pragma once
#include "stdafx.h"
#include "DataReader.h"
#include "DataWriter.h"
#include "commandArgUtil.h"
#include "UCIParser.h"
#include <string>
#include <map>
#include <vector>
#include "minibatchsourcehelpers.h"

static inline size_t RoundUp(size_t m, size_t n)
{
    if (m % n == 0) return m / n;
    else return m / n + 1;
}

namespace Microsoft { namespace MSR { namespace CNTK {

    enum LabelKind
    {
        labelNone = 0,  // no labels to worry about
        labelCategory = 1, // category labels, creates mapping tables
        labelRegression = 2,  // regression labels
        labelOther = 3, // some other type of label
    };

    template<class ElemType>
    class UCIFastReader : public IDataReader<ElemType>
    {
    public:
        using LabelType = typename IDataReader<ElemType>::LabelType;
        using LabelIdType = typename IDataReader<ElemType>::LabelIdType;
        using IDataReader<ElemType>::mBlgSize;
        //typedef std::string LabelType;
        //typedef unsigned LabelIdType;
    private:
        UCIParser<ElemType, LabelType> m_parser;
        size_t m_mbSize;    // size of minibatch requested
        LabelIdType m_labelIdMax; // maximum label ID we have encountered so far
        LabelIdType m_labelDim; // maximum label ID we will ever see (used for array dimensions)
        size_t m_mbStartSample; // starting sample # of the next minibatch
        size_t m_epochSize; // size of an epoch
        size_t m_epoch; // which epoch are we on
        size_t m_epochStartSample; // the starting sample for the epoch
        size_t m_totalSamples;  // number of samples in the dataset
        size_t m_randomizeRange; // randomization range
        size_t m_featureCount; // feature count
        size_t m_readNextSample; // next sample to read
        bool m_labelFirst;  // the label is the first element in a line
        bool m_partialMinibatch;    // a partial minibatch is allowed
        LabelKind m_labelType;  // labels are categories, create mapping table
        msra::dbn::randomordering m_randomordering;   // randomizing class

        std::wstring m_labelsName;
        std::wstring m_featuresName;
        std::wstring m_labelsCategoryName;
        std::wstring m_labelsMapName;
        ElemType* m_featuresBuffer;
        ElemType* m_labelsBuffer;
        LabelIdType* m_labelsIdBuffer;
        std::wstring m_labelFileToWrite;  // set to the path if we need to write out the label file

        bool m_endReached;
        int m_traceLevel;

        // feature and label data are parallel arrays
        std::vector<ElemType> m_featureData;
        std::vector<LabelIdType> m_labelIdData;
        std::vector<LabelType> m_labelData;
        MBLayoutPtr m_pMBLayout;

        // map is from ElemType to LabelType
        // For UCI, we really only need an int for label data, but we have to transmit in Matrix, so use ElemType instead
        std::map<LabelIdType, LabelType> m_mapIdToLabel;
        std::map<LabelType, LabelIdType> m_mapLabelToId;

        /**
        for reading one line per file, i.e., a file has only one line of data
        */
        bool mOneLinePerFile;
    
        // caching support
        DataReader<ElemType>* m_cachingReader;
        DataWriter<ElemType>* m_cachingWriter;
        ConfigParameters m_readerConfig;
        void InitCache(const ConfigParameters& config);

        size_t RandomizeSweep(size_t epochSample);
        bool Randomize() {return m_randomizeRange != randomizeNone;}
        size_t UpdateDataVariables(size_t mbStartSample);
        void SetupEpoch();
        void StoreLabel(ElemType& labelStore, const LabelType& labelValue);
        size_t RecordsToRead(size_t mbStartSample, bool tail=false);
        void ReleaseMemory();
        void WriteLabelFile();

        virtual bool EnsureDataAvailable(size_t mbStartSample, bool endOfDataCheck=false);
        virtual bool ReadRecord(size_t readSample);
    public:
        virtual void Init(const ConfigParameters& config);
        virtual void Destroy();
        UCIFastReader() { m_featuresBuffer=NULL; m_labelsBuffer=NULL; m_labelsIdBuffer=NULL; m_pMBLayout=make_shared<MBLayout>(); }
        virtual ~UCIFastReader();
        virtual void StartMinibatchLoop(size_t mbSize, size_t epoch, size_t requestedEpochSamples=requestDataSize);
        virtual bool GetMinibatch(std::map<std::wstring, Matrix<ElemType>*>& matrices);

        size_t GetNumParallelSequences() { return m_pMBLayout->GetNumParallelSequences(); }
        void CopyMBLayoutTo(MBLayoutPtr pMBLayout) { pMBLayout->CopyFrom(m_pMBLayout); };
        virtual const std::map<LabelIdType, LabelType>& GetLabelMapping(const std::wstring& sectionName);
        virtual void SetLabelMapping(const std::wstring& sectionName, const std::map<LabelIdType, LabelType>& labelMapping);
        virtual bool GetData(const std::wstring& sectionName, size_t numRecords, void* data, size_t& dataBufferSize, size_t recordStart=0);

        virtual bool DataEnd(EndDataType endDataType);
        void SetSentenceSegBatch(Matrix<float>&, Matrix<ElemType>&) { };

        void SetNumParallelSequences(const size_t sz);

        void SetRandomSeed(int) { NOT_IMPLEMENTED;  }
    };

}}}
