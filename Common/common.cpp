#include "common.h"
#include "log.hpp"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <limits>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/date_time.hpp>

NAMESPACE_PROLOG
using namespace google::protobuf;
using namespace google::protobuf::io;

int64 generateSequence()
{	
	boost::random::random_device rng;
	boost::random::uniform_int_distribution<int32> dist(0, (std::numeric_limits<int32>::max)());
	return dist(rng) + (++sequence);
}

static std::string chars(
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"1234567890"
	// "!@#$%^&*()"
	// "`~-_=+[{]}\\|;:'\",<.>/?"
	);

int generateNumberInRange(int min, int max)
{
	boost::random::random_device rng;
	boost::random::uniform_int_distribution<> range_dist(min, max);
	return range_dist(rng);
}

std::string generateName(int length)
{
	std::string result;
	boost::random::random_device rng;
	boost::random::uniform_int_distribution<> index_dist(0, chars.size() - 1);
	for (int i = 0; i < length; ++ i)
		result.append(1, chars[index_dist(rng)]);
	return result;
}

bool Decode(UNOMsg &msg,const std::string& data)
{
	CodedInputStream coded_input((const uint8 *)data.c_str(), data.length());
	uint32 size;
	if (!coded_input.ReadVarint32(&size))
	{
		LOG(warning) << "read varint32 failed.";
		return false;
	}
	
	CodedInputStream::Limit limit = coded_input.PushLimit(size);
	if (!msg.ParseFromCodedStream(&coded_input))
	{
		LOG(warning) << "ParseFromCodedStream failed.";
		return false;
	}

	if (!coded_input.ConsumedEntireMessage())
	{
		LOG(warning) << "ConsumedEntireMessage failed.";
		return false;
	}
	
	coded_input.PopLimit(limit);
	return true;
}

bool Encode(ScoreMsg *smsg, std::string &data, int64 sequence)
{
	UNOMsg msg;
	msg.set_type(UNOMsg::Score);
	msg.set_sequence(sequence);
	msg.set_allocated_score_msg(smsg);
	return Encode(msg, data);
}

bool Encode(RoomMsg *rmsg, std::string &data, int64 sequence)
{
	UNOMsg msg;
	msg.set_type(UNOMsg::Room);
	msg.set_sequence(sequence);
	msg.set_allocated_room_msg(rmsg);
	return Encode(msg, data);
}

bool Encode(GameMsg *gmsg, std::string &data, int64 sequence)
{
	UNOMsg msg;
	msg.set_type(UNOMsg::Game);
	msg.set_sequence(sequence);
	msg.set_allocated_game_msg(gmsg);
	return Encode(msg, data);
}

bool Encode(ChatMsg *cmsg, std::string &data, int64 sequence)
{
	UNOMsg msg;
	msg.set_type(UNOMsg::Chat);
	msg.set_sequence(sequence);
	msg.set_allocated_chat_msg(cmsg);
	return Encode(msg, data);
}

bool Encode(const UNOMsg &msg, std::string& data)
{
	if (!msg.IsInitialized())
	{
		LOG(warning) << " msg is not property initialized. ";
		return false;
	}
	
	std::ostringstream oss;
	OstreamOutputStream output(&oss, msg.ByteSize() + CodedOutputStream::VarintSize32(msg.ByteSize()));
	CodedOutputStream coded_output(&output);
	coded_output.WriteVarint32(msg.ByteSize());
	if (!msg.SerializeToCodedStream(&coded_output))
	{
		LOG(warning) << "SerializeToCodedStream failed.";
		return false;
	}
	
	
	char *buffer;
	int size;
	if (!output.Next((void **)&buffer, &size))
	{
		LOG(warning) << "OstreamOutputStream Next failed.";
		return false;
	}

	data.assign(buffer, size);
	return true;
}

boost::posix_time::ptime epoch(boost::gregorian::date(1970, boost::gregorian::Jan, 1));
std::string getTime(int64 microseconds)
{
	boost::posix_time::ptime now = epoch + boost::posix_time::microseconds(microseconds);
	return boost::posix_time::to_simple_string(now);
}

int64 getTimestamp()
{
	boost::posix_time::time_duration time_from_epoch = 
		boost::posix_time::microsec_clock::universal_time() - epoch;
	return time_from_epoch.total_microseconds();
}

// todo: read from language list
std::string TextSpecification[] = 
{
	"waiting for black operation, play, draw, done is forbidden",			// 0
	"two card not same",													// 1
	"black card can not play 2",											// 2
	"play 2 card after draw card",											// 3
	"does not have this card",												// 4
	"interception failed",													// 5
	"normally play wrong card",												// 6
	"play card not same to drawn card, or this is a functional card.",		// 7
	"has been punished, but dishonesty",									// 8
	"not his turn to draw card",											// 9
	"end turn without draw or play card",									// 10
	"yell uno but card count is not 1",										// 11
	"doubt uno failed, hahaha",												// 12
	"timeout, turn to next one",											// 13
	"plus 4 doubt failed",													// 14
	"waiting for assign color, but send doubt info",						// 15
	"waiting for assign plus 4 target, but send color info",				// 16
	"was doubt uno by someone sucessfully",									// 17
	"was assigned by plus 4, hahahah",										// 18
	"plus 4 doubt success",													// 19
	"not in black operation period",										// 20
	"choose a color now",													// 21
	"assign someone now"													// 22
};

NAMESPACE_EPILOG