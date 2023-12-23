/* ************************************************************************
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and constants                *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#pragma once

#include "net.h"

/**********************************************************************
* Structures                                                          *
**********************************************************************/
template<typename T>
class AttributeManager {
public:
    T get(const std::string& category, const std::string& name, const T& defaultValue = T()) {
        auto cat_it = attributes.find(category);
        if (cat_it != attributes.end()) {
            auto& nameMap = cat_it->second;
            auto name_it = nameMap.find(name);
            if (name_it != nameMap.end()) {
                return name_it->second;
            }
        }
        return defaultValue;
    }
    void set(const std::string& category, const std::string& name, const T& value) {
        attributes[category][name] = value;
    }
    void remove(const std::string& category, const std::string& name) {
        auto cat_it = attributes.find(category);
        if (cat_it != attributes.end()) {
            cat_it->second.erase(name);
            if (cat_it->second.empty()) {
                attributes.erase(category);
            }
        }
    }
    bool has(const std::string& category, const std::string& name) {
        auto cat_it = attributes.find(category);
        if (cat_it != attributes.end()) {
            return cat_it->second.find(name) != cat_it->second.end();
        }
        return false;
    }
    std::vector<std::string> listCategories() {
        std::vector<std::string> categories;
        for (const auto& pair : attributes) {
            categories.push_back(pair.first);
        }
        return categories;
    }
    std::vector<std::string> listEntries(const std::string& category) {
        std::vector<std::string> entries;
        auto cat_it = attributes.find(category);
        if (cat_it != attributes.end()) {
            for (const auto& pair : cat_it->second) {
                entries.push_back(pair.first);
            }
        }
        return entries;
    }

    nlohmann::json serialize() {
        nlohmann::json j;
        for (const auto& cat : attributes) {
            for (const auto& pair : cat.second) {
                j[cat.first][pair.first] = pair.second;
            }
        }
        return j;
    }

    void deserialize(nlohmann::json& j) {
        attributes.clear();
        for (auto cat_it = j.begin(); cat_it != j.end(); ++cat_it) {
            for (auto name_it = cat_it->begin(); name_it != cat_it->end(); ++name_it) {
                attributes[cat_it.key()][name_it.key()] = name_it.value();
            }
        }
    }

private:
    std::unordered_map<std::string, std::unordered_map<std::string, T>> attributes;
};

struct CompiledScript {
    const std::string_view code;
    const std::shared_ptr<std::string> bytecode;
};

class GameObject;

class GameModule {
    friend class GameObject;
public:
    GameModule(std::filesystem::path folder);
    const std::string& getName() const;
    std::weak_ptr<GameObject> getGameObject(int64_t id, int64_t generation = -1) const;
    std::weak_ptr<GameObject> createGameObject(int64_t id = -1, int64_t generation = -1);
    const std::map<int64_t, std::shared_ptr<GameObject>>& getGameObjects() const;
    void saveAll();
protected:
    void markDirty(int64_t id);
private:
    std::string name;
    std::map<int64_t, std::shared_ptr<GameObject>> m_gameObjects;
    std::set<int64_t> dirtyObjects;
    std::filesystem::path folder;
};

class EventHandler {
public:

};

class GameObject {
public:
    GameObject(GameModule *module, int64_t id, int64_t generation);
    std::string renderID() const;

    nlohmann::json serialize();
    void deserialize(const nlohmann::json& j);

    std::shared_ptr<GameObject> getRelation(const std::string& name) const;
    void setRelation(const std::string& name, std::shared_ptr<GameObject> parent);

    std::set<std::shared_ptr<GameObject>> getReverseRelation(const std::string& name) const;

    GameModule* getModule() const;

    std::shared_ptr<GameObject> getOwner() const;
    void setOwner(std::shared_ptr<GameObject> owner);

    std::shared_ptr<GameObject> getParent() const;
    void setParent(std::shared_ptr<GameObject> parent);

private:
    GameModule* m_module;
    int64_t m_id, m_generation;

    std::weak_ptr<GameObject> m_parent;
    std::weak_ptr<GameObject> m_owner;

    std::unordered_map<std::string, std::weak_ptr<GameObject>> m_relations;
    std::unordered_map<std::string, std::set<std::weak_ptr<GameObject>>> m_reverseRelations;

};


class PlayView;
class PlayViewParser {
public:
    PlayViewParser(PlayView *desc) : desc(desc) {}
    virtual ~PlayViewParser() = default;
    virtual void start() = 0;
    virtual bool parse(const std::string &input) = 0;
    virtual bool resume() = 0;
protected:
    PlayView *desc;
};


class PlayView {
public:
    PlayView(std::shared_ptr<GameObject> character);
    void onConnectionLost(int64_t);
    void onConnectionClosed(int64_t);

    void handle_input();
    void start();
    void handleLostLastConnection(bool graceful);
    void sendText(const std::string &txt);
    void addParser(std::shared_ptr<PlayViewParser> parser);
    void update(double deltaTime);
    bool isActive();
private:
    std::weak_ptr<GameObject> character;
    std::weak_ptr<GameObject> puppet;

    int64_t id{-1};
    std::map<int64_t, std::shared_ptr<net::Connection>> conns;
    std::list<std::shared_ptr<PlayViewParser>> parserStack;


    bool has_prompt{true};        /* is the user at a prompt?             */
    std::string last_input;        /* the last input			*/
    std::unique_ptr<net::Channel<std::string>> raw_input_queue;        /* queue of raw unprocessed input		*/
    std::list<std::string> input_queue;
    std::string output;        /* ptr to the current output buffer	*/
    std::list<std::string> history;        /* History of commands, for ! mostly.	*/

};
