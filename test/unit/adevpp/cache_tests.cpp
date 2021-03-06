
#include <iostream>
#include <string>
#include <cstring>
#include <gtest/gtest.h>
#include <adevpp/containers/cache.hpp>

using namespace std;

struct DummyStruct{
    std::string dude;
};


// instanciate and play with gates
TEST(AdevppTests, CacheTest){



    Adevpp::Cache<std::string,  DummyStruct> cache;

    ASSERT_TRUE(cache.find("hello").get() == NULL);

    boost::shared_ptr<DummyStruct> dumb( new DummyStruct());
    dumb->dude = "bob";

    ASSERT_STREQ("bob",cache.insert("hello", dumb)->dude.c_str());


    ASSERT_STREQ("bob", cache.find("hello")->dude.c_str());
    ASSERT_EQ(1, cache.getSize());
    ASSERT_STREQ("bob", cache.take("hello")->dude.c_str());
    ASSERT_EQ(0, cache.getSize());

    ASSERT_TRUE(cache.find("hello").get() == NULL);

    ASSERT_STREQ("bob",cache.insert("alice", dumb)->dude.c_str());

    ASSERT_STREQ("bob",cache.insert("test", dumb)->dude.c_str());
    ASSERT_STREQ("bob",cache.insert("john", dumb)->dude.c_str());

    ASSERT_STREQ("bob", cache.find("john").get()->dude.c_str());

    ASSERT_EQ(3, cache.getSize());

    ASSERT_TRUE(cache.erase("john"));
    ASSERT_FALSE(cache.erase("john"));

    ASSERT_EQ(2, cache.getSize());
    ASSERT_TRUE( cache.find("john").get() == NULL);

    cache.clear();
    ASSERT_EQ(0, cache.getSize());
}

