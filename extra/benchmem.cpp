/**
* reports transfert rate.
*
*  g++-fsf-4.7 -Ofast -mavx -g3  -o benchmem benchmem.cpp
*/
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <iostream>
#include <limits>
#include <immintrin.h>
#include <vector>
using namespace std;


double mysecond()
{
        struct timeval tp;
        struct timezone tzp;
        int i;

        i = gettimeofday(&tp,&tzp);
        return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

class WallClockTimer {
public:
    struct timeval t1, t2;
    WallClockTimer() :
        t1(), t2() {
        gettimeofday(&t1, 0);
        t2 = t1;
    }
    void reset() {
        gettimeofday(&t1, 0);
        t2 = t1;
    }
    int elapsed() {
        return (t2.tv_sec * 1000 + t2.tv_usec / 1000) - (t1.tv_sec * 1000 + t1.tv_usec / 1000);
    }
    int split() {
        gettimeofday(&t2, 0);
        return elapsed();
    }
};

void fastcopy(const int * in , int * out, size_t N) {// assumes N is a multiple of 4
    const __m128i * min = reinterpret_cast<const __m128i *>(in); 
    __m128i * mout = reinterpret_cast<__m128i *>(out);
    
    for(size_t k = 0; k < N/4; ++k) {
              _mm_store_si128(mout++,  _mm_load_si128(min++));
    }
}

void ufastcopy(const int * in , int * out, size_t N) {// assumes N is a multiple of 4
  const __m128i * min = reinterpret_cast<const __m128i *>(in);
  const __m128i * minMax = min + N/4;
  const __m128i * minMax4 = min + 4*(N/16);
  __m128i * mout = reinterpret_cast<__m128i *>(out);

  while (min < minMax4) {
          _mm_store_si128(mout++,  _mm_load_si128(min++));
          _mm_store_si128(mout++,  _mm_load_si128(min++));
          _mm_store_si128(mout++,  _mm_load_si128(min++));
          _mm_store_si128(mout++,  _mm_load_si128(min++));
  }
  while (min < minMax) {
          _mm_store_si128(mout++,  _mm_load_si128(min++));
  }
}

#define STREAM_ARRAY_SIZE 10000000


static int	A[STREAM_ARRAY_SIZE],
			B[STREAM_ARRAY_SIZE];

int stackrun() {
	double besttime0 = numeric_limits<double>::max();
    WallClockTimer t;
     
    for(int k = 0; k<20;++k) {
        t.reset();
        for(int z = 0 ; z < STREAM_ARRAY_SIZE ; ++z) 
        	   B[z] = A[z];
        
        double thistime0 = t.split()/1000.0;

        if(thistime0 < besttime0) besttime0 = thistime0;

    }
    cout<<" naive = "<<STREAM_ARRAY_SIZE/(1000.0*1000.0*besttime0) 
        <<" mis or "<< STREAM_ARRAY_SIZE*sizeof(int)/(1024.0*1024.0*besttime0)<<" MB/s"<<endl;
    return B[0];

}


int mallocrun(const size_t N) {
    cout<<" N = "<<N<<endl;
    int * a = (int *) malloc(N*sizeof(int));
    int * b = (int *) malloc(N*sizeof(int));
    
    WallClockTimer t;
    int total = 0;
    double besttime0 = numeric_limits<double>::max();
    double besttime1 = numeric_limits<double>::max();
    double besttime2 = numeric_limits<double>::max();
    double besttime3 = numeric_limits<double>::max();
    double besttime4 = numeric_limits<double>::max();
     
    for(int k = 0; k<20;++k) {
        t.reset();
        fastcopy(b,a,N);
        double thistime0 = t.split()/1000.0;
        if(thistime0 < besttime0) besttime0 = thistime0;
        t.reset();
        memset(a,k,N*sizeof(int));
        double thistime1 = t.split()/1000.0;
        if(thistime1 < besttime1) besttime1 = thistime1;
        t.reset();
        memcpy(b,a,N*sizeof(int));
        double thistime2 = t.split()/1000.0;
        if(thistime2 < besttime2) besttime2 = thistime2;
        t.reset();
        ufastcopy(b,a,N);
        double thistime3 = t.split()/1000.0;
        if(thistime3 < besttime3) besttime3 = thistime3;
        
        t.reset();
        for(int z = 0 ; z < N ; ++z) 
        	   b[z] = a[z];
        double thistime4 = t.split()/1000.0;

        if(thistime4 < besttime4) besttime4 = thistime4;
        
    }
    free(a);
    free(b);
    cout<<" naive = "<<N/(1000.0*1000.0*besttime4) 
        <<" mis or "<< N*sizeof(int)/(1024.0*1024.0*besttime4)<<" MB/s"<<endl;
    cout<<" Fast SIMD memcpy speed = "<<N/(1000.0*1000.0*besttime3) 
        <<" mis or "<< N*sizeof(int)/(1024.0*1024.0*besttime3)<<" MB/s"<<endl;
    cout<<" SIMD memcpy speed = "<<N/(1000.0*1000.0*besttime0) 
        <<" mis or "<< N*sizeof(int)/(1024.0*1024.0*besttime0)<<" MB/s"<<endl;
    cout<<" memset speed = "<<N/(1000.0*1000.0*besttime1) 
        <<" mis or "<< N*sizeof(int)/(1024.0*1024.0*besttime1)<<" MB/s"<<endl;
    cout<<" memcpy speed = "<<N/(1000.0*1000.0*besttime2) 
        <<" mis or "<< N*sizeof(int)/(1024.0*1024.0*besttime2)<<" MB/s"<<endl;
    return total;
}




int run(const size_t N, const size_t howmany) {
    cout<<" N = "<<N<<" volume = "<<N*howmany<<endl;
    vector<vector<int> > a (howmany);
    vector<vector<int> > b (howmany);
    for(size_t k = 0 ; k < howmany; ++k) {
        a[k].resize(N,k);
        b[k].resize(N);
    }

    WallClockTimer t;
    int total = 0;
    double besttime0 = numeric_limits<double>::max();
    double besttime1 = numeric_limits<double>::max();
    double besttime2 = numeric_limits<double>::max();
    double besttime3 = numeric_limits<double>::max();
    double besttime4 = numeric_limits<double>::max();
    double besttime5 = numeric_limits<double>::max();
     
    for(int k = 0; k<20;++k) {
        t.reset();
        for(int L = 0; L < howmany; ++L) {
          fastcopy(&b[L][0],&a[L][0],N);
        }
        double thistime0 = t.split()/1000.0;
        if(thistime0 < besttime0) besttime0 = thistime0;
        t.reset();
        for(int L = 0; L < howmany; ++L) {
          memset(&a[L][0],k,N*sizeof(int));
        }
        double thistime1 = t.split()/1000.0;
        if(thistime1 < besttime1) besttime1 = thistime1;
        t.reset();
        for(int L = 0; L < howmany; ++L) {
          memcpy(&b[L][0],&a[L][0],N*sizeof(int));
        }
        double thistime2 = t.split()/1000.0;
        if(thistime2 < besttime2) besttime2 = thistime2;
        total += b[0][N-2];
        t.reset();
        for(int L = 0; L < howmany; ++L) {
          ufastcopy(&b[L][0],&a[L][0],N);
        }
        double thistime3 = t.split()/1000.0;
        if(thistime3 < besttime3) besttime3 = thistime3;
        
        t.reset();
        for(int L = 0; L < howmany; ++L) {
        	for(int z = 0 ; z < N ; ++z) 
        	   b[L][z] = a[L][z];
        }
        total += b[0][N-2] + b[howmany-1][N-3];
        double thistime4 = t.split()/1000.0;

        if(thistime4 < besttime4) besttime4 = thistime4;
        double bef = mysecond();
        for(int L = 0; L < howmany; ++L) {
        	for(int z = 0 ; z < N ; ++z) 
        	   b[L][z] = a[L][z];
        }
        double thistime5 = mysecond() - bef;
        if(thistime5 < besttime5) besttime5 = thistime5;

    }
    cout<<" naive = "<<N*howmany/(1000.0*1000.0*besttime5) 
        <<" mis or "<< N*howmany*sizeof(int)/(1024.0*1024.0*besttime5)<<" MB/s"<<endl;

    cout<<" naive = "<<N*howmany/(1000.0*1000.0*besttime4) 
        <<" mis or "<< N*howmany*sizeof(int)/(1024.0*1024.0*besttime4)<<" MB/s"<<endl;
    cout<<" Fast SIMD memcpy speed = "<<N*howmany/(1000.0*1000.0*besttime3) 
        <<" mis or "<< N*howmany*sizeof(int)/(1024.0*1024.0*besttime3)<<" MB/s"<<endl;
    cout<<" SIMD memcpy speed = "<<N*howmany/(1000.0*1000.0*besttime0) 
        <<" mis or "<< N*howmany*sizeof(int)/(1024.0*1024.0*besttime0)<<" MB/s"<<endl;
    cout<<" memset speed = "<<N*howmany/(1000.0*1000.0*besttime1) 
        <<" mis or "<< N*howmany*sizeof(int)/(1024.0*1024.0*besttime1)<<" MB/s"<<endl;
    cout<<" memcpy speed = "<<N*howmany/(1000.0*1000.0*besttime2) 
        <<" mis or "<< N*howmany*sizeof(int)/(1024.0*1024.0*besttime2)<<" MB/s"<<endl;
    return total;
}

int main() {
	cout<<"Stack:"<<endl;
	stackrun();
	cout<<"Malloc:"<<endl;
	mallocrun(10000000);	
	cout<<"STL:"<<endl;
    run(100000,100);
	run(1000000,10);
	run(10000000,1);	
	return 0;
}
