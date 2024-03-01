/*
* @file mimo-pca-test.cpp
* @author Ward Nijman
*
* @brief unit-test of mimo.pca
*
* @copyright Copyright (C) 2016 by IRCAM – Centre Pompidou, Paris, France.
* All rights reserved.

* @overview This unit-test uses input matrices generated in matlab. The output is compared
* to the output from matlab to check it's validity. From test 1 till 6 the output matrices
* VT, S and U are compared and afterwards the forward and backward transformation are
* compared. From test 6 till 10 we feed the program matrices with either identical columns
* or identical rows. Only the first dimension should be consistent in the resulting matrix
* of singular values. Thus we check the whole matrix of S and a forward transformation with
* only the the first relevant column of V. Because VT and U differ between different SVD 
* calculations the matrices are compared by their respective absolute approximate.
*/

#include "../catch.hpp"

#define NONORMALIZATION
//#define WIN32

#include "mimo_pca.h"
#include "PiPoTestReceiver.h"
#include <iostream>
#include <fstream>
#include <tuple>
#include <string>
#include <algorithm>

using namespace std; // for cout, etc.

// overload function in mimo_pca.h to work for std::vector
std::vector<float> xTranspose (std::vector<float> &in, int m, int n)
{
    return MiMoPca::xTranspose(in.data(), m, n);
}

std::vector<float> xMul(float *left, std::vector<float> &right, int m, int n, int p)
{
    return MiMoPca::xMul(left, right.data(), m, n, p);
}


std::vector<float> xCrop (const std::vector<float>& in, unsigned int oldrow, unsigned int oldcol, unsigned int newrow, unsigned int newcol)
{
    if (oldrow == newrow && oldcol == newcol)
        return in;
    std::vector<float> out(newrow*newcol);
    for (unsigned int row = 0; row < newrow; ++row)
        for (unsigned int col = 0; col < newcol; ++col)
        {
            unsigned int oldindex = col + (row * oldcol);
            unsigned int newindex = col + (row * newcol);
            out[newindex] = in[oldindex];
        }
    return out;
}

std::tuple<std::vector<float>,int,int> parseMatrix(std::string path)
{
    std::string file_path = __FILE__;
    file_path = file_path.substr(0, file_path.rfind("/")) + "/";

    std::ifstream instream(file_path + path);
    std::vector<std::string> header;
    std::vector<float> vals;
    long rows, cols;

    for(std::string in; std::getline(instream,in); )
    {
        if(std::strncmp(in.c_str(), "#", 1) == 0)
        {
            header.push_back(in);
            if(header.size() == 5)
            {
                rows = std::strtol(header[3].data()+8,NULL,10);
                cols = std::strtol(header[4].data()+11,NULL,10);
                vals.reserve(rows*cols);
            }
        }
        else
        {
            std::istringstream ss(in);
            std::istream_iterator<std::string> begin(ss), end;
            std::vector<std::string> elements(begin, end);
            for(auto e : elements)
                vals.push_back((float)std::strtod(e.c_str(),NULL));
        }
    }
    std::tuple<std::vector<float>,int,int> out = std::make_tuple(vals, rows, cols);
    return vals.size() < 1 ? std::tuple<std::vector<float>,int,int>() : out;
}

std::tuple<std::vector<float>,int,int> m1 = parseMatrix("pca-matlab-test/output/m1.txt");
std::tuple<std::vector<float>,int,int> m2 = parseMatrix("pca-matlab-test/output/m2.txt");
std::tuple<std::vector<float>,int,int> m3 = parseMatrix("pca-matlab-test/output/m3.txt");
std::tuple<std::vector<float>,int,int> m4 = parseMatrix("pca-matlab-test/output/m4.txt");
std::tuple<std::vector<float>,int,int> m5 = parseMatrix("pca-matlab-test/output/m5.txt");
std::tuple<std::vector<float>,int,int> m6 = parseMatrix("pca-matlab-test/output/m6.txt");
std::tuple<std::vector<float>,int,int> m7 = parseMatrix("pca-matlab-test/output/m7.txt");
std::tuple<std::vector<float>,int,int> m8 = parseMatrix("pca-matlab-test/output/m8.txt");
std::tuple<std::vector<float>,int,int> m9 = parseMatrix("pca-matlab-test/output/m9.txt");
std::tuple<std::vector<float>,int,int> m10 = parseMatrix("pca-matlab-test/output/m10.txt");
std::tuple<std::vector<float>,int,int> u1 = parseMatrix("pca-matlab-test/output/u1.txt");
std::tuple<std::vector<float>,int,int> u2 = parseMatrix("pca-matlab-test/output/u2.txt");
std::tuple<std::vector<float>,int,int> u3 = parseMatrix("pca-matlab-test/output/u3.txt");
std::tuple<std::vector<float>,int,int> u4 = parseMatrix("pca-matlab-test/output/u4.txt");
std::tuple<std::vector<float>,int,int> u5 = parseMatrix("pca-matlab-test/output/u5.txt");
std::tuple<std::vector<float>,int,int> u6 = parseMatrix("pca-matlab-test/output/u6.txt");
std::tuple<std::vector<float>,int,int> u7 = parseMatrix("pca-matlab-test/output/u7.txt");
std::tuple<std::vector<float>,int,int> u8 = parseMatrix("pca-matlab-test/output/u8.txt");
std::tuple<std::vector<float>,int,int> u9 = parseMatrix("pca-matlab-test/output/u9.txt");
std::tuple<std::vector<float>,int,int> u10 = parseMatrix("pca-matlab-test/output/u10.txt");
std::tuple<std::vector<float>,int,int> s1 = parseMatrix("pca-matlab-test/output/s1.txt");
std::tuple<std::vector<float>,int,int> s2 = parseMatrix("pca-matlab-test/output/s2.txt");
std::tuple<std::vector<float>,int,int> s3 = parseMatrix("pca-matlab-test/output/s3.txt");
std::tuple<std::vector<float>,int,int> s4 = parseMatrix("pca-matlab-test/output/s4.txt");
std::tuple<std::vector<float>,int,int> s5 = parseMatrix("pca-matlab-test/output/s5.txt");
std::tuple<std::vector<float>,int,int> s6 = parseMatrix("pca-matlab-test/output/s6.txt");
std::tuple<std::vector<float>,int,int> s7 = parseMatrix("pca-matlab-test/output/s7.txt");
std::tuple<std::vector<float>,int,int> s8 = parseMatrix("pca-matlab-test/output/s8.txt");
std::tuple<std::vector<float>,int,int> s9 = parseMatrix("pca-matlab-test/output/s9.txt");
std::tuple<std::vector<float>,int,int> s10 = parseMatrix("pca-matlab-test/output/s10.txt");
std::tuple<std::vector<float>,int,int> v1 = parseMatrix("pca-matlab-test/output/v1.txt");
std::tuple<std::vector<float>,int,int> v2 = parseMatrix("pca-matlab-test/output/v2.txt");
std::tuple<std::vector<float>,int,int> v3 = parseMatrix("pca-matlab-test/output/v3.txt");
std::tuple<std::vector<float>,int,int> v4 = parseMatrix("pca-matlab-test/output/v4.txt");
std::tuple<std::vector<float>,int,int> v5 = parseMatrix("pca-matlab-test/output/v5.txt");
std::tuple<std::vector<float>,int,int> v6 = parseMatrix("pca-matlab-test/output/v6.txt");
std::tuple<std::vector<float>,int,int> v7 = parseMatrix("pca-matlab-test/output/v7.txt");
std::tuple<std::vector<float>,int,int> v8 = parseMatrix("pca-matlab-test/output/v8.txt");
std::tuple<std::vector<float>,int,int> v9 = parseMatrix("pca-matlab-test/output/v9.txt");
std::tuple<std::vector<float>,int,int> v10 = parseMatrix("pca-matlab-test/output/v10.txt");
std::tuple<std::vector<float>,int,int> vlm1 = parseMatrix("pca-matlab-test/output/vlm1.txt");
std::tuple<std::vector<float>,int,int> vlm2 = parseMatrix("pca-matlab-test/output/vlm2.txt");
std::tuple<std::vector<float>,int,int> vlm3 = parseMatrix("pca-matlab-test/output/vlm3.txt");
std::tuple<std::vector<float>,int,int> vlm4 = parseMatrix("pca-matlab-test/output/vlm4.txt");
std::tuple<std::vector<float>,int,int> vlm5 = parseMatrix("pca-matlab-test/output/vlm5.txt");
std::tuple<std::vector<float>,int,int> vlm6 = parseMatrix("pca-matlab-test/output/vlm6.txt");
std::tuple<std::vector<float>,int,int> vlm7 = parseMatrix("pca-matlab-test/output/vlm7.txt");
std::tuple<std::vector<float>,int,int> vlm8 = parseMatrix("pca-matlab-test/output/vlm8.txt");
std::tuple<std::vector<float>,int,int> vlm9 = parseMatrix("pca-matlab-test/output/vlm9.txt");
std::tuple<std::vector<float>,int,int> vlm10 = parseMatrix("pca-matlab-test/output/vlm10.txt");
std::tuple<std::vector<float>,int,int> fwtest1 = parseMatrix("pca-matlab-test/output/vectest1.txt");
std::tuple<std::vector<float>,int,int> fwtest2 = parseMatrix("pca-matlab-test/output/vectest2.txt");
std::tuple<std::vector<float>,int,int> fwtest3 = parseMatrix("pca-matlab-test/output/vectest3.txt");
std::tuple<std::vector<float>,int,int> fwtest4 = parseMatrix("pca-matlab-test/output/vectest4.txt");
std::tuple<std::vector<float>,int,int> fwtest5 = parseMatrix("pca-matlab-test/output/vectest5.txt");
std::tuple<std::vector<float>,int,int> fwtest6 = parseMatrix("pca-matlab-test/output/vectest6.txt");
std::tuple<std::vector<float>,int,int> fwtest7 = parseMatrix("pca-matlab-test/output/vectest7.txt");
std::tuple<std::vector<float>,int,int> fwtest8 = parseMatrix("pca-matlab-test/output/vectest8.txt");
std::tuple<std::vector<float>,int,int> fwtest9 = parseMatrix("pca-matlab-test/output/vectest9.txt");
std::tuple<std::vector<float>,int,int> fwtest10 = parseMatrix("pca-matlab-test/output/vectest10.txt");
std::tuple<std::vector<float>,int,int> fw1 = parseMatrix("pca-matlab-test/output/fw1.txt");
std::tuple<std::vector<float>,int,int> fw2 = parseMatrix("pca-matlab-test/output/fw2.txt");
std::tuple<std::vector<float>,int,int> fw3 = parseMatrix("pca-matlab-test/output/fw3.txt");
std::tuple<std::vector<float>,int,int> fw4 = parseMatrix("pca-matlab-test/output/fw4.txt");
std::tuple<std::vector<float>,int,int> fw5 = parseMatrix("pca-matlab-test/output/fw5.txt");
std::tuple<std::vector<float>,int,int> fw6 = parseMatrix("pca-matlab-test/output/fw6.txt");
std::tuple<std::vector<float>,int,int> fw7 = parseMatrix("pca-matlab-test/output/fw7.txt");
std::tuple<std::vector<float>,int,int> fw8 = parseMatrix("pca-matlab-test/output/fw8.txt");
std::tuple<std::vector<float>,int,int> fw9 = parseMatrix("pca-matlab-test/output/fw9.txt");
std::tuple<std::vector<float>,int,int> fw10 = parseMatrix("pca-matlab-test/output/fw10.txt");
std::tuple<std::vector<float>,int,int> bwtest1 = parseMatrix("pca-matlab-test/output/bw1test.txt");
std::tuple<std::vector<float>,int,int> bwtest2 = parseMatrix("pca-matlab-test/output/bw2test.txt");
std::tuple<std::vector<float>,int,int> bwtest3 = parseMatrix("pca-matlab-test/output/bw3test.txt");
std::tuple<std::vector<float>,int,int> bwtest4 = parseMatrix("pca-matlab-test/output/bw4test.txt");
std::tuple<std::vector<float>,int,int> bwtest5 = parseMatrix("pca-matlab-test/output/bw5test.txt");
std::tuple<std::vector<float>,int,int> bwtest6 = parseMatrix("pca-matlab-test/output/bw6test.txt");
std::tuple<std::vector<float>,int,int> bwtest7 = parseMatrix("pca-matlab-test/output/bw7test.txt");
std::tuple<std::vector<float>,int,int> bwtest8 = parseMatrix("pca-matlab-test/output/bw8test.txt");
std::tuple<std::vector<float>,int,int> bwtest9 = parseMatrix("pca-matlab-test/output/bw9test.txt");
std::tuple<std::vector<float>,int,int> bwtest10 = parseMatrix("pca-matlab-test/output/bw10test.txt");
std::tuple<std::vector<float>,int,int> bw1 = parseMatrix("pca-matlab-test/output/bw1.txt");
std::tuple<std::vector<float>,int,int> bw2 = parseMatrix("pca-matlab-test/output/bw2.txt");
std::tuple<std::vector<float>,int,int> bw3 = parseMatrix("pca-matlab-test/output/bw3.txt");
std::tuple<std::vector<float>,int,int> bw4 = parseMatrix("pca-matlab-test/output/bw4.txt");
std::tuple<std::vector<float>,int,int> bw5 = parseMatrix("pca-matlab-test/output/bw5.txt");
std::tuple<std::vector<float>,int,int> bw6 = parseMatrix("pca-matlab-test/output/bw6.txt");
std::tuple<std::vector<float>,int,int> bw7 = parseMatrix("pca-matlab-test/output/bw7.txt");
std::tuple<std::vector<float>,int,int> bw8 = parseMatrix("pca-matlab-test/output/bw8.txt");
std::tuple<std::vector<float>,int,int> bw9 = parseMatrix("pca-matlab-test/output/bw9.txt");
std::tuple<std::vector<float>,int,int> bw10 = parseMatrix("pca-matlab-test/output/bw10.txt");


// **** helper function for generating a pca-setup and first iteration

int decompose(unsigned int m, unsigned int n, MiMoPca& pca, std::tuple<std::vector<float>,int,int> matrix)
{
    unsigned int* sizes = new unsigned int(m);
    const PiPoStreamAttributes* testattr = new const PiPoStreamAttributes(false, 44100,0, 1, n, NULL, false, 0, m, false);
    mimo_buffer* testbuffer = new mimo_buffer;
    testbuffer->numframes = m;
    testbuffer->data = std::get<0>(matrix).data();
    testbuffer->varsize = NULL;
    testbuffer->has_timetags = false;
    testbuffer->time.starttime = 0;
    pca.setup(1, 1, (int*)sizes, &testattr);
    pca.train(1, 0, 1, testbuffer);
    delete testbuffer;
    delete testattr;
    delete sizes;
    return 1;
}

bool vecIsAbsAprox(float* left, float* right, unsigned long leftsize)
{
    for(unsigned long i = 0; i < leftsize; ++i)
    {
        float epsilon = fabs(fabs(left[i]) - (fabs(right[i])));
        if(epsilon > 0.01)
            return false;
    }
    return true;
}

bool vecIsAbsAprox(const std::vector<float>& left, const std::vector<float>& right)
{
    if (left.size() != right.size())
    {
	cout << "size mismatch " << left.size() << " vs. " << right.size() << endl;
	return false;
    }
    
    for(unsigned int i = 0; i < left.size(); ++i)
    {
        float epsilon = fabs(fabs(left[i]) - (fabs(right[i])));
        if (epsilon > 0.01)
	{
	    cout << "mismatch at index " << i << " of " << left.size() << ": " << epsilon << endl;
	    for (size_t j = 0; j < left.size(); j++)
		cout << (j == 0 ? "[" : " ") << left[j] << (j == left.size() - 1  ?  "]"  :  "") << endl;
	    for (size_t j = 0; j < right.size(); j++)
		cout << (j == 0 ? "[" : " ") << right[j] << (j == right.size() - 1  ?  "]"  :  "") << endl;	    
            return false;
	}
    }
    return true;
}

TEST_CASE("mimo-pca")
{
    PiPoTestReceiver parent(NULL);

    GIVEN("4 x 2 matrix lozenge")
    {
        MiMoPca pca(&parent);
        unsigned int rank = 2;
        pca.rank_attr_.set(rank);

	std::tuple<std::vector<float>,int,int> lozenge   = {{0, 0,  1, 0,  1, 1,  2, 1}, 4, 2};
	std::tuple<std::vector<float>,int,int> lozenge_v = {{0.8507, -0.5257, 0.5257, 0.8507}, 2, 2}; // V should be n x n loadings, values from loadings_test.m
	std::tuple<std::vector<float>,int,int> lozenge_s = {{1.6180, 0.6180}, 2, 1};
	std::tuple<std::vector<float>,int,int> lozenge_fw = {{-1.1135,    0.1004,
							      -0.2629,   -0.4253,
							       0.2629,    0.4253,
							       1.1135,   -0.1004}, 4, 2};

	unsigned int m = std::get<1>(lozenge);
        unsigned int n = std::get<2>(lozenge);
        
        THEN("Decomposition and transformation should rotate matrix")
        {
	    char buf[2048];
            REQUIRE(decompose(m, n, pca, lozenge));
            CHECK(vecIsAbsAprox(std::get<0>(lozenge_v), pca.V_)); // check v
            CHECK(vecIsAbsAprox(std::get<0>(lozenge_s), pca.S_)); // check s

            pca.setReceiver(&parent);
            pca.forwardbackward_attr_.set((PiPo::Enumerate) 0); //forward transformation
	    pca.model_attr_.setJson(pca.getmodel()->to_json(buf, 2048)); //forward transformation
            pca.streamAttributes(false, 44100, 0, n, 1, NULL, false, 0, m);
            pca.frames(0, 0, std::get<0>(lozenge).data(), n, m);
            CHECK(vecIsAbsAprox(parent.values, std::get<0>(lozenge_fw).data(), n * m));
           
            //because our feature space is slightly different we reassign VT from matlab
            pca.decomposition_.VT = xTranspose(std::get<0>(vlm1), n, rank); //??????
            pca.forwardbackward_attr_.set(1); // backward transformation
            pca.streamAttributes(false, 44100, 0, rank, 1, NULL, false, 0, 1);
            pca.frames(0, 0, std::get<0>(bwtest1).data(), rank, 1);
            CHECK(vecIsAbsAprox(parent.values, std::get<0>(bw1).data(), std::get<0>(bw1).size()));
        }
    }
    return;
    
    GIVEN("A M*N input matrix m1:")
    {
        MiMoPca pca(&parent);
        unsigned int rank = 10;
        pca.rank_attr_.set(rank);
        unsigned int m = std::get<1>(m1);
        unsigned int n = std::get<2>(m1);
        
        THEN("Decomposition and transformation should result in")
        {
            REQUIRE(decompose(m, n, pca, m1));
            REQUIRE(vecIsAbsAprox(std::get<0>(vlm1), pca.decomposition_.V)); //check v
            CHECK(vecIsAbsAprox(xCrop(std::get<0>(u1), m, m, m, rank), pca.U_)); // check u
            CHECK(vecIsAbsAprox(std::get<0>(s1), pca.S_)); // check s

            pca.setReceiver(&parent);
            pca.forwardbackward_attr_.set((PiPo::Enumerate) 0); //forward transformation
            pca.streamAttributes(false, 44100, 0, n, 1, NULL, false, 0, 1);
            pca.frames(0, 0, std::get<0>(fwtest1).data(), n, 1);
            CHECK(vecIsAbsAprox(parent.values, std::get<0>(fw1).data(), std::get<0>(fw1).size()));
           
            //because our feature space is slightly different we reassign VT from matlab
            pca.decomposition_.VT = xTranspose(std::get<0>(vlm1), n, rank);
            pca.forwardbackward_attr_.set(1); // backward transformation
            pca.streamAttributes(false, 44100, 0, rank, 1, NULL, false, 0, 1);
            pca.frames(0, 0, std::get<0>(bwtest1).data(), rank, 1);
            CHECK(vecIsAbsAprox(parent.values, std::get<0>(bw1).data(), std::get<0>(bw1).size()));
        }
    }
    GIVEN("A M*M square input matrix m2:")
    {
        MiMoPca pca(&parent);
        unsigned int rank = 10;
        pca.rank_attr_.set(rank);
        unsigned int m = std::get<1>(m2);
        unsigned int n = std::get<2>(m2);
        
        THEN("Decomposition and transformation should result in")
        {
            REQUIRE(decompose(m, n, pca, m2));
            REQUIRE(vecIsAbsAprox(std::get<0>(vlm2), pca.decomposition_.V)); //check v
            CHECK(vecIsAbsAprox(xCrop(std::get<0>(u2), m, m, m, rank), pca.U_)); // check u
            CHECK(vecIsAbsAprox(std::get<0>(s2), pca.S_)); // check s
            
            pca.setReceiver(&parent);
            pca.forwardbackward_attr_.set((PiPo::Enumerate) 0); //forward transformation
            pca.streamAttributes(false, 44100, 0, n, 1, NULL, false, 0, 1);
            pca.frames(0, 0, std::get<0>(fwtest2).data(), n, 1);
            CHECK(vecIsAbsAprox(parent.values, std::get<0>(fw2).data(), std::get<0>(fw2).size()));
            
            pca.decomposition_.VT = xTranspose(std::get<0>(vlm2), n, rank); //reassign VT
            pca.forwardbackward_attr_.set(1); // backward transformation
            pca.streamAttributes(false, 44100, 0, rank, 1, NULL, false, 0, 1);
            pca.frames(0, 0, std::get<0>(bwtest2).data(), rank, 1);
            CHECK(vecIsAbsAprox(parent.values, std::get<0>(bw2).data(), std::get<0>(bw2).size()));
        }
    }
    GIVEN("A M*M diagonal input matrix m3:")
    {
        //because Octave doesnt include zero's in diagonal matrices we first add those
        std::vector<float> diagvals = std::get<0>(m3);
        std::vector<float>& matrix3 = std::get<0>(m3);
        matrix3.clear();
        matrix3.resize(std::get<1>(m3) * std::get<2>(m3));
        for(int i = 0, j = 0; i < std::get<1>(m3) * std::get<2>(m3); ++j, i+=(std::get<1>(m3) + 1))
            matrix3[i] = diagvals[j];
        
        MiMoPca pca(&parent);
        unsigned int rank = 10;
        pca.rank_attr_.set(rank);
        unsigned int m = std::get<1>(m3);
        unsigned int n = std::get<2>(m3);
        
        THEN("Decomposition and transformation should result in")
        {
            REQUIRE(decompose(m, n, pca, m3));
            REQUIRE(vecIsAbsAprox(std::get<0>(vlm3), pca.decomposition_.V)); //check v
            CHECK(vecIsAbsAprox(xCrop(std::get<0>(u3), m, m, m, rank), pca.U_)); // check u
            CHECK(vecIsAbsAprox(std::get<0>(s3), pca.S_)); // check s
            
            pca.setReceiver(&parent);
            pca.forwardbackward_attr_.set((PiPo::Enumerate) 0); //forward transformation
            pca.streamAttributes(false, 44100, 0, n, 1, NULL, false, 0, 1);
            pca.frames(0, 0, std::get<0>(fwtest3).data(), n, 1);
            CHECK(vecIsAbsAprox(parent.values, std::get<0>(fw3).data(), std::get<0>(fw3).size()));
            
            pca.decomposition_.VT = xTranspose(std::get<0>(vlm3), n, rank); //reassign VT
            pca.forwardbackward_attr_.set(1); // backward transformation
            pca.streamAttributes(false, 44100, 0, rank, 1, NULL, false, 0, 1);
            pca.frames(0, 0, std::get<0>(bwtest3).data(), rank, 1);
            CHECK(vecIsAbsAprox(parent.values, std::get<0>(bw3).data(), std::get<0>(bw3).size()));
        }
    }
    GIVEN("a M>N rectangular input matrix m4:")
    {
        MiMoPca pca(&parent);
        unsigned int rank = 10;
        pca.rank_attr_.set(rank);
        unsigned int m = std::get<1>(m4);
        unsigned int n = std::get<2>(m4);
        
        THEN("Decomposition and transformation should result in")
        {
            REQUIRE(decompose(m, n, pca, m4));
            REQUIRE(vecIsAbsAprox(std::get<0>(vlm4), pca.decomposition_.V)); //check v
            CHECK(vecIsAbsAprox(xCrop(std::get<0>(u4), m, m, m, rank), pca.U_)); // check u
            CHECK(vecIsAbsAprox(std::get<0>(s4), pca.S_)); // check s
            
            pca.setReceiver(&parent);
            pca.forwardbackward_attr_.set((PiPo::Enumerate) 0); //forward transformation
            pca.streamAttributes(false, 44100, 0, n, 1, NULL, false, 0, 1);
            pca.frames(0, 0, std::get<0>(fwtest4).data(), n, 1);
            CHECK(vecIsAbsAprox(parent.values, std::get<0>(fw4).data(), std::get<0>(fw4).size()));
            
            pca.decomposition_.VT = xTranspose(std::get<0>(vlm4), n, rank); //reassign VT
            pca.forwardbackward_attr_.set(1); // backward transformation
            pca.streamAttributes(false, 44100, 0, rank, 1, NULL, false, 0, 1);
            pca.frames(0, 0, std::get<0>(bwtest4).data(), rank, 1);
            CHECK(vecIsAbsAprox(parent.values, std::get<0>(bw4).data(), std::get<0>(bw4).size()));
        }
    }
    GIVEN("a M<N rectangular input matrix m5:")
    {
        MiMoPca pca(&parent);
        unsigned int rank = 10;
        pca.rank_attr_.set(rank);
        unsigned int m = std::get<1>(m5);
        unsigned int n = std::get<2>(m5);
        
        THEN("decomposition and transformation should result in")
        {
            REQUIRE(decompose(m, n, pca, m5));
            REQUIRE(vecIsAbsAprox(std::get<0>(vlm5), pca.decomposition_.V)); //check v
            CHECK(vecIsAbsAprox(xCrop(std::get<0>(u5), m, m, m, rank), pca.U_)); // check u
            CHECK(vecIsAbsAprox(std::get<0>(s5), pca.S_)); // check s
            
            pca.setReceiver(&parent);
            pca.forwardbackward_attr_.set((PiPo::Enumerate) 0); //forward transformation
            pca.streamAttributes(false, 44100, 0, n, 1, NULL, false, 0, 1);
            pca.frames(0, 0, std::get<0>(fwtest5).data(), n, 1);
            CHECK(vecIsAbsAprox(parent.values, std::get<0>(fw5).data(), std::get<0>(fw5).size()));
            
            pca.decomposition_.VT = xTranspose(std::get<0>(vlm5), n, rank); //reassign VT
            pca.forwardbackward_attr_.set(1); // backward transformation
            pca.streamAttributes(false, 44100, 0, rank, 1, NULL, false, 0, 1);
            pca.frames(0, 0, std::get<0>(bwtest5).data(), rank, 1);
            CHECK(vecIsAbsAprox(parent.values, std::get<0>(bw5).data(), std::get<0>(bw5).size()));
        }
    }
    GIVEN("a M*M diagonal input matrix m6:")
    {
        std::vector<float> diagvals = std::get<0>(m6);
        std::vector<float>& matrix6 = std::get<0>(m6);

        matrix6.clear();
        matrix6.resize(std::get<1>(m6)*std::get<2>(m6));
        for(int i = 0, j = 0; i < std::get<1>(m6) * std::get<2>(m6); ++j, i+=(std::get<1>(m6) + 1))
            matrix6[i] = diagvals[j];
        
        MiMoPca pca(&parent);
        unsigned int rank = 10;
        pca.rank_attr_.set(rank);
        unsigned int m = std::get<1>(m6);
        unsigned int n = std::get<2>(m6);
        
        THEN("decomposition and transformation should result in")
        {
            REQUIRE(decompose(m, n, pca, m6));
            REQUIRE(vecIsAbsAprox(std::get<0>(vlm6), pca.decomposition_.V)); //check v
            CHECK(vecIsAbsAprox(xCrop(std::get<0>(u6), m, m, m, rank), pca.U_)); // check u
            CHECK(vecIsAbsAprox(std::get<0>(s6), pca.S_)); // check s
            
            pca.setReceiver(&parent);
            pca.forwardbackward_attr_.set((PiPo::Enumerate) 0); //forward transformation
            pca.streamAttributes(false, 44100, 0, n, 1, NULL, false, 0, 1);
            pca.frames(0, 0, std::get<0>(fwtest6).data(), n, 1);
            CHECK(vecIsAbsAprox(parent.values, std::get<0>(fw6).data(), std::get<0>(fw6).size()));
            
            pca.decomposition_.VT = xTranspose(std::get<0>(vlm6), n, rank); //reassign VT
            pca.forwardbackward_attr_.set(1); // backward transformation
            pca.streamAttributes(false, 44100, 0, rank, 1, NULL, false, 0, 1);
            pca.frames(0, 0, std::get<0>(bwtest6).data(), rank, 1);
            CHECK(vecIsAbsAprox(parent.values, std::get<0>(bw6).data(), std::get<0>(bw6).size()));
        }
    }
    
    GIVEN("a matrix with same cols, square m7:")
    {
        MiMoPca pca(&parent);
        unsigned int rank = 1;
        pca.rank_attr_.set(rank);
        unsigned int m = std::get<1>(m7);
        unsigned int n = std::get<2>(m7);
        
        THEN("decomposition and transformation should result in")
        {
            REQUIRE(decompose(m, n, pca, m7));
            CHECK(vecIsAbsAprox(std::get<0>(s7), pca.S_)); // check s
            pca.setReceiver(&parent);
            pca.forwardbackward_attr_.set((PiPo::Enumerate) 0); //forward transformation
            //crop V because we didn't crop to rank 1 in matlab
            std::get<0>(vlm7) = xCrop(std::get<0>(vlm7), n, n, n, rank);
            //recalculate the forward transform
            std::vector<float> fw7rank1 = xMul( std::get<0>(fwtest7).data(), std::get<0>(vlm7), 1, n, rank);
            pca.streamAttributes(false, 44100, 0, n, 1, NULL, false, 0, 1);
            pca.frames(0, 0, std::get<0>(fwtest7).data(), n, 1);
            CHECK(vecIsAbsAprox(parent.values, fw7rank1.data(), rank));
        }
    }
    GIVEN("a matrix with same cols, square m8:")
    {
        MiMoPca pca(&parent);
        unsigned int rank = 1;
        pca.rank_attr_.set(rank);
        unsigned int m = std::get<1>(m8);
        unsigned int n = std::get<2>(m8);
        
        THEN("decomposition and transformation should result in")
        {
            REQUIRE(decompose(m, n, pca, m8));
            CHECK(vecIsAbsAprox(std::get<0>(s8), pca.S_)); // check s
            pca.setReceiver(&parent);
            pca.forwardbackward_attr_.set((PiPo::Enumerate) 0); //forward transformation
            std::get<0>(vlm8) = xCrop(std::get<0>(vlm8), n, n, n, rank);
            std::vector<float> fw8rank1 = xMul( std::get<0>(fwtest8).data(), std::get<0>(vlm8), 1, n, rank);
            pca.streamAttributes(false, 44100, 0, n, 1, NULL, false, 0, 1);
            pca.frames(0, 0, std::get<0>(fwtest8).data(), n, 1);
            CHECK(vecIsAbsAprox(parent.values, fw8rank1.data(), rank));
        }
    }
    GIVEN("a matrix with same cols, rect m9:")
    {
        MiMoPca pca(&parent);
        unsigned int rank = 1;
        pca.rank_attr_.set(rank);
        unsigned int m = std::get<1>(m9);
        unsigned int n = std::get<2>(m9);
        
        THEN("decomposition and transformation should result in")
        {
            REQUIRE(decompose(m, n, pca, m9));
            CHECK(vecIsAbsAprox(std::get<0>(s9), pca.S_)); // check s
            pca.setReceiver(&parent);
            pca.forwardbackward_attr_.set((PiPo::Enumerate) 0); //forward transformation
            std::get<0>(vlm9) = xCrop(std::get<0>(vlm9), n, n, n, rank);
            std::vector<float> fw9rank1 = xMul( std::get<0>(fwtest9).data(), std::get<0>(vlm9), 1, n, rank);
            pca.streamAttributes(false, 44100, 0, n, 1, NULL, false, 0, 1);
            pca.frames(0, 0, std::get<0>(fwtest9).data(), n, 1);
            CHECK(vecIsAbsAprox(parent.values, fw9rank1.data(), rank));
        }
    }
    GIVEN("a matrix with same cols, rect m10:")
    {
        MiMoPca pca(&parent);
        unsigned int rank = 1;
        pca.rank_attr_.set(rank);
        unsigned int m = std::get<1>(m10);
        unsigned int n = std::get<2>(m10);
        
        THEN("Decomposition and transformation should result in")
        {
            REQUIRE(decompose(m, n, pca, m10));
            CHECK(vecIsAbsAprox(std::get<0>(s10), pca.S_)); // check s
            pca.setReceiver(&parent);
            pca.forwardbackward_attr_.set((PiPo::Enumerate) 0); //forward transformation
            std::get<0>(vlm10) = xCrop(std::get<0>(vlm10), n, n, n, rank);
            std::vector<float> fw10rank1 = xMul( std::get<0>(fwtest10).data(), std::get<0>(vlm10), 1, n, rank);
            pca.streamAttributes(false, 44100, 0, n, 1, NULL, false, 0, 1);
            pca.frames(0, 0, std::get<0>(fwtest10).data(), n, 1);
            CHECK(vecIsAbsAprox(parent.values, fw10rank1.data(), rank));
        }
    }
    //Special case zero matrix: any arbritary U and VT are correct, S should result in a zero matrix.
    GIVEN("A zero matrix, square m11:")
    {
        MiMoPca pca(&parent);
        unsigned int rank = 10;
        pca.rank_attr_.set(rank);
        unsigned int m = 10;
        unsigned int n = 10;
        std::tuple<std::vector<float>,int,int> zeroes = std::make_tuple(std::vector<float>(100, 0),10,10);
        THEN("Decomposition and transformation should result in")
        {
            REQUIRE(decompose(m, n, pca, zeroes));
            CHECK(vecIsAbsAprox(pca.S_, std::get<0>(zeroes))); // check s
        }
    }
}
