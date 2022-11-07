#include "BoundedBuffer.h"

using namespace std;


BoundedBuffer::BoundedBuffer (int _cap) : cap(_cap) {
    // modify as needed
}

BoundedBuffer::~BoundedBuffer () {
    // modify as needed
}

void BoundedBuffer::push (char* msg, int size) {
    // 1. Convert the incoming byte sequence given by msg and size into a vector<char>
    vector<char> msg_(msg, msg + size);

    /*for (int i = 0; i < size; i++) {
        msg_.push_back(msg[i]);
    }*/
    
    // 2. Wait until there is room in the queue (i.e., queue lengh is less than cap)
    std::unique_lock<std::mutex> lk(m);
    size_t cap_ = cap;
    overflow_guard.wait(lk, [this, cap_] { return q.size() < cap_; });
    
    // 3. Then push the vector at the end of the queue
    q.push(msg_);
    lk.unlock();

    // 4. Wake up threads that were waiting for push
    underflow_guard.notify_one();
}

int BoundedBuffer::pop (char* msg, int size) {
    // 1. Wait until the queue has at least 1 item
    std::unique_lock<std::mutex> lk(m);
    underflow_guard.wait(lk, [this] { return q.size() > 0; });

    // 2. Pop the front item of the queue. The popped item is a vector<char>
    vector<char> msg_ = q.front();
    q.pop();
    lk.unlock();

    // 3. Convert the popped vector<char> into a char*, copy that into msg; assert that the vector<char>'s length is <= size
    size_t size_ = size;
    assert(msg_.size() <= size_);
    //memcpy (msg, msg_.data(), msg_.size()); //might need to change
    for (size_t i = 0; i < msg_.size(); i++)
    {
        msg[i] = msg_.at(i);
    }
    

    // 4. Wake up threads that were waiting for pop
    overflow_guard.notify_one();

    // 5. Return the vector's length to the caller so that they know how many bytes were popped
    return msg_.size();
}

size_t BoundedBuffer::size () {
    return q.size();
}
