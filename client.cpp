#include <fstream>
#include <iostream>
#include <thread>
#include <sys/time.h>
#include <sys/wait.h>

#include "BoundedBuffer.h"
#include "common.h"
#include "Histogram.h"
#include "HistogramCollection.h"
#include "FIFORequestChannel.h"

// ecgno to use for datamsgs
#define EGCNO 1

using namespace std;


void patient_thread_function (int patientNo, int requestNum, BoundedBuffer* request_buffer) {
    // functionality of the patient threads
    char buf[MAX_MESSAGE];
    for (int i = 0; i < requestNum; i++)
    {
        datamsg x(patientNo, 0.004*i, EGCNO);
        memcpy(buf, &x, sizeof(datamsg));
        int size = sizeof(datamsg); //each char is 1 byte
        request_buffer->push(buf, size);
    }
    
}

void file_thread_function (/* add necessary arguments */) {
    // functionality of the file thread
}

void worker_thread_function (BoundedBuffer* request_buffer, BoundedBuffer* response_buffer, FIFORequestChannel* chan) {
    // functionality of the worker threads
    for (;;) {
        char buf[MAX_MESSAGE];
        int size = request_buffer->pop(buf, MAX_MESSAGE);

        MESSAGE_TYPE m = *((MESSAGE_TYPE*) buf);
        if (m == DATA_MSG) { //may need to change to f == ""
            double reply;
            chan->cread(buf, sizeof(datamsg)); // question
			chan->cread(&reply, sizeof(double)); //answer
            datamsg* d = (datamsg*) buf;
            pair<int, double> pair_;
            pair_.first = d->person;
            pair_.second = reply;
            response_buffer->push( (char*) &pair_, sizeof(pair<int, double>)/sizeof(char) );
        }
        else if (m == FILE_MSG) {
            size++;
        }
        else if (m == QUIT_MSG) break;
    }
}

void histogram_thread_function (BoundedBuffer* response_buffer, HistogramCollection* hc) {
    // functionality of the histogram threads

    for (;;)
    {
        char buf[MAX_MESSAGE];
        response_buffer->pop(buf, MAX_MESSAGE);
        MESSAGE_TYPE m = *((MESSAGE_TYPE*) buf);
        if (m == QUIT_MSG) break;
        pair<int, double>* pair_ = (pair<int, double>*) buf;
        hc->update(pair_->first, pair_->second);
    }
    
}


int main (int argc, char* argv[]) {
    int n = 1000;	// default number of requests per "patient"
    int p = 10;		// number of patients [1,15]
    int w = 100;	// default number of worker threads
	int h = 20;		// default number of histogram threads
    int b = 20;		// default capacity of the request buffer (should be changed)
	int m = MAX_MESSAGE;	// default capacity of the message buffer
	string f = "";	// name of file to be transferred
    
    // read arguments
    int opt;
	while ((opt = getopt(argc, argv, "n:p:w:h:b:m:f:")) != -1) {
		switch (opt) {
			case 'n':
				n = atoi(optarg);
                break;
			case 'p':
				p = atoi(optarg);
                break;
			case 'w':
				w = atoi(optarg);
                break;
			case 'h':
				h = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
                break;
			case 'm':
				m = atoi(optarg);
                break;
			case 'f':
				f = optarg;
                break;
		}
	}
    
	// fork and exec the server
    int pid = fork();
    if (pid == 0) {
        execl("./server", "./server", "-m", (char*) to_string(m).c_str(), nullptr);
    }
    
	// initialize overhead (including the control channel)
	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
    BoundedBuffer response_buffer(b);
	HistogramCollection hc;

    vector<thread> producers;
    if (f == "") producers.resize(p);
    else producers.resize(1);
    
    vector<FIFORequestChannel*> channels;
    channels.resize(w);

    vector<thread> workers;
    workers.resize(w);

    vector<thread> hist;
    if (f == "") producers.resize(h);
    else producers.resize(0);


    // making histograms and adding to collection
    for (int i = 0; i < p; i++) {
        Histogram* h = new Histogram(10, -2.0, 2.0);
        hc.add(h);
    }
	
	// record start time
    struct timeval start, end;
    gettimeofday(&start, 0);

    /* create all threads here */
    if (f == "")
    {
        for (int i = 1; i <= p; i++)
        {
            producers.push_back(thread(patient_thread_function, i, n, request_buffer));
        }

        for (int i = 0; i < w; i++)
        {
            MESSAGE_TYPE nc = NEWCHANNEL_MSG;
            chan->cwrite(&nc, sizeof(MESSAGE_TYPE));
            char buf0[MAX_MESSAGE];
            chan->cread(buf0, MAX_MESSAGE);
            FIFORequestChannel* chan0 = new FIFORequestChannel(buf0, FIFORequestChannel::CLIENT_SIDE);
            channels.push_back(chan0);

            //workers.push_back(thread(worker_thread_function, ));
        }
        
        for (int i = 0; i < h; i++)
        {
            //hist.push_back(thread(histogram_thread_function, ));
        }
    }
    else {
        //producers.push_back(thread(file_thread_function, ));

        for (int i = 0; i < w; i++)
        {
            //workers.push_back(thread(worker_thread_function, ));

            MESSAGE_TYPE nc = NEWCHANNEL_MSG;
            chan->cwrite(&nc, sizeof(MESSAGE_TYPE));
            char buf0[MAX_MESSAGE];
            chan->cread(buf0, MAX_MESSAGE);
            FIFORequestChannel* chan0 = new FIFORequestChannel(buf0, FIFORequestChannel::CLIENT_SIDE);
            channels.push_back(chan0);
        }
        
    }
    

	/* join all threads here */
    for (int i = 0; i < p; i++)
    {
        //producers.at(i).join();
    }
    //All producers are now done
    for (int i = 0; i < w; i++)
    {
        MESSAGE_TYPE mm = QUIT_MSG;
        request_buffer.push((char*) &mm, sizeof(MESSAGE_TYPE));
        //consumers.at(i).join();
    }
    for (int i = 0; i < n*p; i++)
    {
        MESSAGE_TYPE mm = QUIT_MSG;
        response_buffer.push((char*) &mm, sizeof(MESSAGE_TYPE));
    }

    for (int i = 0; i < h; i++)
    {
        //hist.at(i).join();
    }


	// record end time
    gettimeofday(&end, 0);

    // print the results
	if (f == "") {
		hc.print();
	}
    int secs = ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) / ((int) 1e6);
    int usecs = (int) ((1e6*end.tv_sec - 1e6*start.tv_sec) + (end.tv_usec - start.tv_usec)) % ((int) 1e6);
    std::cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    for (size_t i = 0; i < channels.size(); i++)
    {
        MESSAGE_TYPE q = QUIT_MSG;
        channels.at(i)->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
        delete channels.at(i);
    }
    
	// quit and close control channel
    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    std::cout << "All Done!" << endl;
    delete chan;

	// wait for server to exit
	wait(nullptr);
}
