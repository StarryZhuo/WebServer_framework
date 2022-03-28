#include "config.h"

namespace MyServer
{

    ConfigVarBase::ptr Config::LookupBase(const std::string &name)
    {
        auto it = GetDatas().find(name);
        return it == GetDatas().end() ? nullptr : it->second;
    }

    //Config::ConfigVarMap Config::GetDatas();

    //"A.B", 10
    //A:
    // B:10
    // C:str

    //生成list容器，如果node是map类型进入map后返回
    static void ListALLMember(const std::string &prefix, const YAML::Node &node, std::list<std::pair<std::string, const YAML::Node>> &output)
    {
        if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678") != std::string::npos)
        {
            MYSERVER_LOG_ERROR(MYSERVER_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
            return;
        }
        output.push_back(std::make_pair(prefix, node));
        if (node.IsMap())
        {
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                ListALLMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
            }
        }
    }

    //加载yaml里面的数据
    void Config::LoadFromYaml(const YAML::Node &root)
    {
        std::list<std::pair<std::string, const YAML::Node>> all_node;
        ListALLMember("", root, all_node);

        for (auto &i : all_node)
        {
            std::string key = i.first;
            if (key.empty())
            {
                continue;
            }

            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            ConfigVarBase::ptr var = LookupBase(key);

            if (var)
            {
                if (i.second.IsScalar())
                {
                    var->fromString(i.second.Scalar());
                }
                else
                {
                    std::stringstream ss;
                    ss << i.second;
                    var->fromString(ss.str());
                }
            }
        }
    }

}