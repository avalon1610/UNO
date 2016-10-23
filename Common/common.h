#ifndef _UNO_COMMON_H_
#define _UNO_COMMON_H_
#include "defines.h"
#include "treadstone.pb.h"
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread.hpp>
#include <string>

NAMESPACE_PROLOG

typedef google::protobuf::int8 int8;
typedef google::protobuf::int16 int16;
typedef google::protobuf::int32 int32;
typedef google::protobuf::int64 int64;
typedef boost::unique_lock<boost::shared_mutex> writelock;
typedef boost::upgrade_lock<boost::shared_mutex> readlock;
typedef boost::upgrade_to_unique_lock<boost::shared_mutex> upgradelock;

template<typename F>
void AddTask(const F &task)
{
	boost::shared_ptr<boost::thread> thread(new boost::thread(task));
}

typedef int64 Timestamp;
static int64 sequence = 0;
int64 generateSequence();
std::string generateName(int);
int64 getTimestamp();
std::string getTime(int64 microseconds);
int generateNumberInRange(int min, int max);

bool Decode(UNOMsg &msg,const std::string& data);
bool Encode(RoomMsg *rmsg, std::string &data, int64 sequence);
bool Encode(GameMsg *gmsg, std::string &data, int64 sequence);
bool Encode(ChatMsg *cmsg, std::string &data, int64 sequence);
bool Encode(ScoreMsg *smsg, std::string &data, int64 sequence);
bool Encode(const UNOMsg &msg, std::string& data);
std::string TextSpecification[];

NAMESPACE_EPILOG
	
#endif