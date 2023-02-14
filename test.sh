set -ex
bazel-5.3.0 build //src/workerd/server:workerd
mkdir -p test
rm -f ./test/workerd
cp ~/workerd/bazel-bin/src/workerd/server/workerd ./test/
cd test
echo Running...
./workerd --verbose serve my-config.capnp &
sleep 2
curl localhost:8080
killall workerd
