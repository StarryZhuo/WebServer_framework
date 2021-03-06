#include "../MyServer/config.h"
#include "../MyServer/log.h"
#include <yaml-cpp/yaml.h>
#include <iostream>


MyServer::ConfigVar<int>::ptr g_int_value_config = 
    MyServer::Config::Lookup("system.port", (int)8080, "system port");

MyServer::ConfigVar<float>::ptr g_int_value_config1 = 
    MyServer::Config::Lookup("system.port", (float)8080, "system port");
   
MyServer::ConfigVar<float>::ptr g_float_value_config = 
    MyServer::Config::Lookup("system.value", (float)10.2f, "system value");

MyServer::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config = 
    MyServer::Config::Lookup("system.int_vec", std::vector<int>{1,2}, "system int_vec");

MyServer::ConfigVar<std::list<int> >::ptr g_int_list_value_config = 
    MyServer::Config::Lookup("system.int_list", std::list<int>{1,2}, "system int_list");

MyServer::ConfigVar<std::set<int> >::ptr g_int_set_value_config = 
    MyServer::Config::Lookup("system.int_set", std::set<int>{1,2}, "system int_set");

MyServer::ConfigVar<std::unordered_set<int> >::ptr g_int_uset_value_config = 
    MyServer::Config::Lookup("system.int_uset", std::unordered_set<int>{1,2}, "system int_uset");

MyServer::ConfigVar<std::map<std::string, int> >::ptr g_str_int_map_value_config = 
    MyServer::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k",2}, {"l", 3}}, "system str_int_map");

MyServer::ConfigVar<std::unordered_map<std::string, int> >::ptr g_str_int_umap_value_config =
    MyServer::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k",2}}, "system str_int_map");
//遍历yaml,将logs.yaml的信息输出
void print_yaml(const YAML::Node& node, int level) {
    if(node.IsScalar()) { //表示是string的话
        MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if(node.IsNull()) {
        MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - " << node.Type() << " - " << level;
    } else if(node.IsMap()) {
        //map结构的遍历
        for(auto it = node.begin(); it != node.end(); ++it) {
            MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if(node.IsSequence()) {
        for(size_t i = 0; i < node.size(); ++i) {
            MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml() {
    //加载文件
    YAML::Node root = YAML::LoadFile("/home/starry/My_Server_Framework/bin/conf/logs.yml");
    print_yaml(root, 0);

    //MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << root;
}

void test_config() {
    MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << "before:" << g_int_value_config->getValue();
    MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << "before:" << g_float_value_config->toString();

#define XX(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << #prefix " " #name ": "<< i; \
        } \
        MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

#define XX_M(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << #prefix " " #name ": {" \
                    << i.first << " - " << i.second << "}"; \
        } \
        MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_uset_value_config, int_uset, before);
    XX_M(g_str_int_map_value_config, str_int_map, before);
    XX_M(g_str_int_umap_value_config, str_int_umap, before);

//从yaml里面设置输出的结果，使配置生效
    YAML::Node root = YAML::LoadFile("/home/starry/My_Server_Framework/bin/conf/logs.yml");
    MyServer::Config::LoadFromYaml(root);

    MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << "after:" << g_int_value_config->getValue();
    MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << "after:" << g_float_value_config->toString();

    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_uset_value_config, int_uset, after);
    XX_M(g_str_int_map_value_config, str_int_map, after);
    XX_M(g_str_int_umap_value_config, str_int_umap, after);

}

//自定义类型
class Person {
public:
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const {
            std::stringstream ss;
            ss << "[Person name=" << m_name
            << " age=" << m_age
            << " sex=" << m_sex
            << "]";
            return ss.str();
        }

    bool operator==(const Person& oth) const {
        return m_name == oth.m_name
            && m_age == oth.m_age
            && m_sex == oth.m_sex;
    }
};

namespace MyServer {

template <>
class LexicalCast<std::string, Person > {
public:
    Person operator() (const std::string& v) {
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};

template <>
class LexicalCast<Person, std::string> {
public:
    std::string operator() (const Person& p) {
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

}

MyServer::ConfigVar<Person>::ptr g_person = 
    MyServer::Config::Lookup("class.person", Person(), "system person");

MyServer::ConfigVar<std::map<std::string, Person> >::ptr g_person_map = 
    MyServer::Config::Lookup("class.map", std::map<std::string, Person>(), "system person");

MyServer::ConfigVar<std::map<std::string, std::vector<Person> > >::ptr g_person_vec_map = 
    MyServer::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person> >(), "system person");

void test_class() {
    MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << "before:" << g_person->getValue().toString() << " - " << g_person->toString();

#define XX_PM(g_var, prefix) \
    { \
        auto m = g_person_map->getValue(); \
        for(auto& i : m) { \
            MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << prefix << ": " << i.first << " - " << i.second.toString(); \
        } \
        MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << prefix << ": size=" << m.size(); \
    }

    g_person->addListener(10, [](const Person& old_value, const Person& new_value){
        MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << "old_value=" << old_value.toString() << " new value=" << new_value.toString();
    });

    XX_PM(g_person_map, "class.map before");
    MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << "before: " << g_person_vec_map->toString();

    YAML::Node root = YAML::LoadFile("/home/starry/My_Server_Framework/bin/conf/test.yml");
    MyServer::Config::LoadFromYaml(root);

    MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << "after:" << g_person->getValue().toString() << " - " << g_person->toString();
    XX_PM(g_person_map, "class.map after");  
    MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << "after: " << g_person_vec_map->toString();
}

void test_log() {
    static MyServer::Logger::ptr system_log = MYSERVER_LOG_NAME("system");
    //目的是检测用yaml修改过后的日志格式的前后区别
    MYSERVER_LOG_INFO(system_log) << "hello system" << std::endl;
    //toYamlString()将日志器里面的信息通过Yaml载体输出，在log.cc中定义了全局变量，在main之前就定义了，所以更改的Yaml里面的信息
    std::cout << MyServer::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/starry/My_Server_Framework/bin/conf/test.yml");
    MyServer::Config::LoadFromYaml(root);
    std::cout << "=============" << std::endl;
    std::cout << MyServer::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout << "=============" << std::endl;
    std::cout << root << std::endl;
    MYSERVER_LOG_INFO(system_log) << "hello system" << std::endl;

    system_log->setFormatter("%d - %m%n");
    MYSERVER_LOG_INFO(system_log) << "hello system" << std::endl;
}

int main(int argc, char** argv) {
    //test_config();
    //test_class();
    //test_yaml();
    test_log();
    return 0;
}