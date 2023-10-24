#ifndef __SERVER_H_
#define __SERVER_H_

class BenchStatistic {
public:
  BenchStatistic();
  ~BenchStatistic();
  long GetTotalBytes();
  long SetTotalBytes(long value) {total_bytes_received_ = value;}
  long GetTotalTransmitTime();
  long SetTotalTransmitTime(long value) {total_transmit_time_ = value;}
  void Clear();
private:
  long total_bytes_received_;
  long total_transmit_time_;
};

#endif
