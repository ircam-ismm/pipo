    #include "catch.hpp"
#include "mimo_pca.h"
#include <iostream>


MiMoPca pca(NULL);

//svd equivalent WolframAlpha =
//SVD[{{ 4, 4, 5},{ 4, 5, 5},{ 3, 3, 2},{ 4, 5, 4}, {4, 4, 4}, {3, 5, 4}, {4, 4, 3}, {2, 4, 4}, {5, 5, 5 }}];

std::vector<float> xTranspose(float* in, int m, int n)
{
    std::vector<float> out(m*n);
    for(int i = 0; i < m; ++i)
        for(int j = 0; j < n; ++j)
            out[j*m+i] = in[i*n+j];
    return out;
}

float testarray[]={ 4, 4, 5,
                    4, 5, 5,
                    3, 3, 2,
                    4, 5, 4,
                    4, 4, 4,
                    3, 5, 4,
                    4, 4, 3,
                    2, 4, 4,
                    5, 5, 5 };

float testvector[] = {4, 3, 4};
float testvector2[] = {-6.29271841, 0.554258585, 1.04617906};
unsigned int m = 9;
unsigned int n = 3;
unsigned int sizes[] = {m};
mimo_buffer testbuffer;
const PiPoStreamAttributes** testattr = new const PiPoStreamAttributes*[1];
int hasTimeTags = 0;
double rate = 44100;
double offset = 0;
const char **labels = nullptr;
unsigned int numLabels = 0;
bool hasVarSize = false;
double domain = 0;
unsigned int maxFrames = 9;
int ringTail = 0;

TEST_CASE("PCA")
{
    GIVEN("The following setup with matrix: \n 4, 4, 5,\n 4, 5, 5,\n 3, 3, 2,\n 4, 5, 4,\n 4, 4, 4,\n 3, 5, 4,\n 4, 4, 3,\n 2, 4, 4,\n 5, 5, 5 };")
    {
        //setup attributes
        testattr[0] = new const PiPoStreamAttributes(hasTimeTags, rate,offset, 1, n, labels, hasVarSize, domain, maxFrames, ringTail);
        
        //setup mimo_buffer
        testbuffer.numframes = m;
        testbuffer.data = (float*)&testarray;
        testbuffer.varsize = NULL;
        testbuffer.has_timetags = false;
        testbuffer.time.starttime = 0;
        
        //setup and train!
        pca.setup(1, 1, (int*)&sizes, testattr);
        pca.train(1, 0, 1, &testbuffer);
        
        
        THEN("Decomposition should result in:")
        {
            const std::vector<float>& V = pca.decomposition.V;
            const std::vector<float>& VT = pca.decomposition.VT;
            const std::vector<float>& U = pca.U;
            const std::vector<float>& S = pca.S;
//            
//            m = 3;
//            n = 9;
//            unsigned int newsizes[] = {m};
//            std::vector<float> arr = xTranspose(testarray, 9, 3);
//            
//            delete testattr[0];
//            testattr[0] = new const PiPoStreamAttributes(hasTimeTags, rate,offset, 1, n, labels, hasVarSize, domain, maxFrames, ringTail);
//            testbuffer.numframes = m;
//            testbuffer.data = arr.data();
//            pca.setup(1, 1, (int*)&newsizes, testattr);
//            pca.train(1, 0, m, &testbuffer);
            
            std::cout<<"S = " << std::endl;
            for (std::vector<float>::const_iterator i = S.begin(); i != S.end(); ++i)
                std::cout << *i << ' ';
            
        }
        GIVEN("The following inputvector={4, 3, 4}")
        {
            THEN("Forward decoding should result in:")
            {
                //forward decoding
                pca.streamAttributes(hasTimeTags, rate, offset, n, 1, labels, hasVarSize, domain, maxFrames);
                pca.frames(0, 0, testvector, 3, 1);
            }
        }
        GIVEN("The following featurevector={-6.29271841, 0.554258585, 1.04617906}")
        {
            THEN("Backward decoding should result in:")
            {
                //backward decoding
                pca.forwardbackward.set(1.f);
                pca.streamAttributes(hasTimeTags, rate, offset, pca._rank, 1, labels, hasVarSize, domain, maxFrames);
                pca.frames(0, 0, testvector2, 3, 1);
            }
        }
        
        THEN("Writing from json should result in:")
        {
            char* json_output;
            json_output = new char[10000];
            
            //writing to json
            pca.decomposition.to_json(json_output, 10000);
            
            //reading from json
            pca.decomposition.from_json(json_output);
            
        }
    }
}

