/**
 * @file PiPoSel.h
 * @author joseph.larralde@ircam.fr
 * @date 05.01.2016
 *
 * @brief PiPo allowing the selection of a subset of rows or columns
 * in the incoming stream.
 * If no match is found for any selected colum, all columns are passed through.
 * Idem for rows.
 *
 * @ingroup pipomodules
 *
 * @copyright
 * Copyright (C) 2016 by IRCAM – Centre Pompidou, Paris, France.
 * All rights reserved.
 *
 * License (BSD 3-clause)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _PIPO_SEL_H_
#define _PIPO_SEL_H_

#include "PiPo.h"

extern "C"
{
#include <stdlib.h>
}

class PiPoSelect : public PiPo
{
private:
    std::vector<PiPo::Atom> _colNames;
    std::vector<int> _rowIndices;
    std::vector<unsigned int> _usefulColIndices; // sorted and validity-checked indices
    std::vector<unsigned int> _usefulRowIndices; // sorted and validity-checked indices
    
    unsigned int lastNum;
    
    unsigned int frameWidth;
    unsigned int frameHeight;
    
    unsigned int outWidth;
    unsigned int outHeight;
    
    unsigned int outFrameSize;
    
    PiPoValue *outValues;
    
public:
    PiPoVarSizeAttr<PiPo::Atom> colNames;
    PiPoVarSizeAttr<int> rowIndices;
    
    PiPoSelect(Parent *parent, PiPo *receiver = NULL) :
    PiPo(parent, receiver),
    colNames(this, "cols", "List of Names or Indices of Columns to Select", true, 1),
    rowIndices(this, "rows", "List of Indices of Rows to Select", true, 1, 0)
    {
        this->lastNum = 0;
        
        this->frameWidth = 0;
        this->frameHeight = 0;
        this->outWidth = 0;
        this->outHeight = 0;
        this->outFrameSize = 0;
        
        this->outValues = (PiPoValue *)malloc(0);
    }
    
    ~PiPoSelect()
    {
        free(this->outValues);
    }
    
    int streamAttributes(bool hasTimeTags, double rate, double offset, unsigned int width, unsigned int height, const char **labels, bool hasVarSize, double domain, unsigned int maxFrames)
    {
        unsigned int cnSize = this->colNames.getSize();
        unsigned int riSize = this->rowIndices.getSize();
        const char *colNames[128];
        
        unsigned int frameWidth = width;
        unsigned int frameHeight = height;
        
        //================== check col names changes
        bool colNamesChanged = false;
        if(cnSize != this->_colNames.size())
        {
            colNamesChanged = true;
        }
        else
        {
            for(unsigned int i = 0; i < cnSize; i++)
            {
                if(this->_colNames[i] != this->colNames[i])
                {
                    colNamesChanged = true;
                    break;
                }
            }
        }
        
        //================== check col indices changes
        bool rowIndicesChanged = false;
        if(riSize != this->_rowIndices.size())
        {
            rowIndicesChanged = true;
        }
        else
        {
            for(unsigned int i = 0; i < riSize; i++)
            {
                if(this->_rowIndices[i] != this->rowIndices[i])
                {
                    rowIndicesChanged = true;
                    break;
                }
            }
        }
        
        //=========== check any change in stream attributes and/or PiPo attributes =========//
        
        if(colNamesChanged || rowIndicesChanged || frameWidth != this->frameWidth || frameHeight != this->frameHeight)
        {
            /*
             for(unsigned int i = 0; i < this->colNames.size(); i++)
             {
             printf("%s ", this->colNames[i]);
             }
             printf("\n");
             //*/
            
            //copy new col names
            this->_colNames.clear();
            for(unsigned int i = 0; i < cnSize; i++)
            {
                this->_colNames.push_back(this->colNames[i]);
            }
            
            // copy new row indices
            this->_rowIndices.clear();
            for(unsigned int i = 0; i < riSize; i++)
            {
                this->_rowIndices.push_back(this->rowIndices[i]);
            }
            
            // set new input dimensions
            this->frameWidth = frameWidth;
            this->frameHeight = frameHeight;
            
            //============== first deal with col indices ================//
            
            unsigned int cnt = 0;
            this->_usefulColIndices.clear();
            for(unsigned int i = 0; i < cnSize; i++)
            {
                switch(this->_colNames[i].getType())
                {
                    case Double:
                    case Int:
                    {
                        int res = this->_colNames[i].getInt();
                        if(res >= 0 && (unsigned int)res < this->frameWidth)
                        {
                            //add res to the indices of valid columns
                            this->_usefulColIndices.push_back(res);
                            cnt++;
                        }
                    }
                    break;
                        
                    case String:
                    {
                        if(labels != NULL)
                        {
                            for(unsigned int j = 0; j < this->frameWidth; j++)
                            {
                                if(strcmp(this->_colNames[i].getString(), labels[j]) == 0)
                                {
                                    this->_usefulColIndices.push_back(j);
                                    cnt++;
                                }
                            }
                        }
                    }
                    break;
                        
                    default:
                        break;
                }
            }
            /*
            printf("selected cols :");
            for(unsigned int i = 0; i < this->_usefulColIndices.size(); i++)
            {
                printf(" %d", this->_usefulColIndices[i]);
            }
            printf("\n");
            //*/
            
            if(this->_usefulColIndices.size() == 0) // pass all through
            {
                // fill with all indices
                this->_usefulColIndices.resize(this->frameWidth);
                for(unsigned int i = 0; i < this->frameWidth; i++)
                {
                    this->_usefulColIndices[i] = i;
                }
            }
            
            this->outWidth = static_cast<unsigned int>(this->_usefulColIndices.size());
            // default sorting is ascending order
            //std::sort(this->_usefulColIndices.begin(), this->_usefulColIndices.end());
            
            //============== now deal with row indices ================//
            
            this->_usefulRowIndices.clear();
            for(unsigned int i = 0; i < this->_rowIndices.size(); i++)
            {
                if(this->_rowIndices[i] < (int)(this->frameHeight) && this->_rowIndices[i] >= 0) {
                    this->_usefulRowIndices.push_back(this->_rowIndices[i]);
                }
            }
            
            if(this->_usefulRowIndices.size() == 0) // pass all through
            {
                // fill with all indices
                this->_usefulRowIndices.resize(this->frameHeight);
                for(unsigned int i = 0; i < this->frameHeight; i++)
                {
                    this->_usefulRowIndices[i] = i;
                }
            }
            
            this->outHeight = static_cast<unsigned int>(this->_usefulRowIndices.size());
            // default sorting is ascending order
            //std::sort(this->_usefulRowIndices.begin(), this->_usefulRowIndices.end());
            
            this->outFrameSize = this->outWidth * this->outHeight;
        }
        
        for(unsigned int i = 0; i < this->outWidth; i++)
        {
            colNames[i] = (labels != NULL ? labels[this->_usefulColIndices[i]] : "");
        }
        
        return this->propagateStreamAttributes(hasTimeTags, rate, offset, this->outWidth, this->outHeight, (labels != NULL ? colNames : NULL), hasVarSize, domain, maxFrames);
    }
    
    int frames(double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
    {
        if(num != this->lastNum) {
            this->lastNum = num;
            this->outValues = (PiPoValue *)realloc(this->outValues, this->outFrameSize * num * sizeof(PiPoValue));
        }
        
        for(unsigned int n = 0; n < num; n++)
        {
            unsigned int cnt = 0;
            for(unsigned int i = 0; i < this->outHeight; i++)
            {
                for(unsigned int j = 0; j < this->outWidth; j++)
                {
                    this->outValues[n * this->outFrameSize + cnt] = values[this->_usefulRowIndices[i] * this->frameWidth + this->_usefulColIndices[j]];
                    cnt++;
                }
            }
            
            values += size;
        }
        
        return this->propagateFrames(time, weight, this->outValues, this->outFrameSize, num);
    }
};


#endif /* _PIPO_SEL_H_ */
