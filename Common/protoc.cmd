
protoc --cpp_out=. treadstone.proto
cp treadstone.proto ../UNO
cd ../UNO
protogen.cmd