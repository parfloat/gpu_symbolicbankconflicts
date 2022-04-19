#ifndef CUDATIMING_H_
#define CUDATIMING_H_

class CUDATiming{
public:
	void startMeasuring(){
		cudaEventCreate(&start);
		cudaEventCreate(&stop);
		cudaEventRecord(start,0);
	}

	void stopMeasuring(){
		cudaEventRecord(stop,0);
		cudaEventSynchronize(stop);
		cudaEventElapsedTime(&time, start, stop);
	}

	float getTime(){
		return (time);
	}


private:
	cudaEvent_t start, stop;
	float  time;
};

#endif
