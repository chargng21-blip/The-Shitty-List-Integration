#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/web.hpp>
#include "ListParser.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

using namespace geode::prelude;
namespace web = geode::utils::web;

namespace {
constexpr float kWidth = 480.f;
constexpr float kHeight = 305.f;
constexpr float kRowsLeft = 18.f;
constexpr float kRowsRight = 462.f;
constexpr int kPerPage = 6;
constexpr std::array<char const*, 2> kUrls = {
    "https://tsl.pages.dev/list/tsl",
    "https://tsl.pages.dev/list/tslplus"
};

std::vector<tsl::Level> fallback(int tab) {
    // The offline sample is intentionally small. The live site is authoritative.
    if (tab == 0) {
        return {
            {1,"Shitty Acheron (Twoft)","Twoft"},
            {2,"Shitty Nhelv","jackyy"},
            {3,"Shitty Discretion Advised","Keruo"},
            {4,"Shitty SARYYX NEVER CLEAR","Twoft"},
            {5,"Shitty Sky Shredder","phantom"},
            {6,"Shitty Knights of Thunder","Berriez"},
            {7,"Shitty Antarctic Lights","hexurii"},
            {8,"Shitty The Devil","FnMs"},
            {9,"Shitty The Art of the Blade","jackyy"},
            {10,"Shitty Ashley Wave Trials","Rory"}
        };
    }
    return {
        {1,"Shitty Flamewall","Deceit"},
        {2,"Shitty Nullscapes","dofar"},
        {3,"Shitty LIMBO","Asteria."},
        {4,"Shitty Kyouki","Deceit"},
        {5,"Shitty Injury","gzarvati"},
        {6,"Shitty Stellar Night","heizenn"},
        {7,"Shitty Spectre","zEvolution64"},
        {8,"Shitty NEUTRA","/Swan"},
        {9,"Shitty Avernus","mrcsgmd"},
        {10,"Shitty Tidal Wave","DuoFresh"}
    };
}

struct ListCache {
    std::array<std::vector<tsl::Level>, 2> levels{fallback(0), fallback(1)};
    std::array<bool, 2> live{false, false};
    std::array<bool, 2> fetching{false, false};
    std::array<geode::async::TaskHolder<web::WebResponse>, 2> requests;
};

ListCache& cache() {
    static ListCache data;
    return data;
}

void preloadLists() {
    auto& shared = cache();
    for (int tab = 0; tab < 2; ++tab) {
        if (shared.live[tab] || shared.fetching[tab]) continue;
        shared.fetching[tab] = true;
        auto req = web::WebRequest();
        req.userAgent("TSLIntegration/0.1 (Geode; GD)");
        shared.requests[tab].spawn(req.get(kUrls[tab]), [tab](web::WebResponse response) {
            auto& c = cache();
            c.fetching[tab] = false;
            if (!response.ok()) return;
            auto body = response.string();
            if (body.isErr()) return;
            auto parsed = tsl::parseListHTML(body.unwrap());
            if (parsed.empty()) return;
            c.levels[tab] = std::move(parsed);
            c.live[tab] = true;
        });
    }
}

CCLayerColor* rect(CCNode* parent, float x, float y, float w, float h, ccColor4B color) {
    auto panel = CCLayerColor::create(color, w, h);
    panel->setPosition({x, y});
    parent->addChild(panel);
    return panel;
}

CCLabelBMFont* label(CCNode* parent, std::string const& s,
                     float x, float y, float scale = 0.5f,
                     ccColor3B color = {235, 243, 255},
                     float maxWidth = 0.f, bool left = true) {
    auto text = CCLabelBMFont::create(s.c_str(), "bigFont.fnt");
    text->setScale(scale);
    text->setColor(color);
    text->setAnchorPoint(left ? CCPoint{0.f, 0.5f} : CCPoint{0.5f, 0.5f});
    text->setPosition({x, y});
    if (maxWidth > 0.f) text->limitLabelWidth(maxWidth, scale, 0.12f);
    parent->addChild(text);
    return text;
}

CCMenuItemSpriteExtra* button(CCMenu* menu, CCObject* target, SEL_MenuHandler handler,
                              std::string const& text, float x, float y, float width,
                              ccColor3B color = {216, 226, 255}) {
    auto bg = CCLayerColor::create({34, 47, 76, 245}, width, 24.f);
    auto textLabel = label(bg, text, width / 2.f, 12.f, 0.43f, color, width - 8.f, false);
    (void)textLabel;
    auto item = CCMenuItemSpriteExtra::create(bg, target, handler);
    item->setPosition({x, y});
    menu->addChild(item);
    return item;
}

class TSLPopup : public geode::Popup {
    int m_tab = 0;
    int m_page = 0;
    int m_generation = 0;
    bool m_loading = false;
    std::vector<size_t> m_filtered;
    std::string m_search;
    geode::TextInput* m_searchInput = nullptr;
    cocos2d::CCNode* m_rows = nullptr;
    cocos2d::CCMenu* m_menu = nullptr;
    cocos2d::CCLabelBMFont* m_status = nullptr;
    cocos2d::CCLabelBMFont* m_counter = nullptr;
    cocos2d::CCLabelBMFont* m_heading = nullptr;
    cocos2d::CCNode* m_tabHighlight = nullptr;
    geode::async::TaskHolder<web::WebResponse> m_request;

    void setStatus(std::string const& s, ccColor3B color = {149, 169, 198}) {
        if (m_status) {
            m_status->setString(s.c_str());
            m_status->setColor(color);
        }
    }

    void rebuild() {
        if (!m_rows) return;
        m_rows->removeAllChildrenWithCleanup(true);
        m_filtered.clear();
        std::string query = tsl::lowercase(m_search);
        auto const& items = cache().levels[m_tab];
        for (size_t i = 0; i < items.size(); ++i) {
            auto const& lv = items[i];
            std::string hay = tsl::lowercase(lv.name + " " + lv.creator + " " + std::to_string(lv.rank));
            if (query.empty() || hay.find(query) != std::string::npos) m_filtered.push_back(i);
        }
        int pages = std::max(1, static_cast<int>((m_filtered.size() + kPerPage - 1) / kPerPage));
        m_page = std::clamp(m_page, 0, pages - 1);
        m_counter->setString(fmt::format("{}/{}  |  {} levels", m_page + 1, pages, m_filtered.size()).c_str());

        if (m_filtered.empty()) {
            label(m_rows, "no levels found", 230.f, 101.f, 0.52f, {170, 184, 214}, 240.f, false);
            return;
        }
        auto rowsMenu = CCMenu::create();
        rowsMenu->setPosition({0.f, 0.f});
        m_rows->addChild(rowsMenu);
        auto start = static_cast<size_t>(m_page) * kPerPage;
        for (size_t index = 0; index < kPerPage && start + index < m_filtered.size(); ++index) {
            auto idx = m_filtered[start + index];
            auto const& lv = items[idx];
            float bottom = 173.f - static_cast<float>(index) * 25.f;
            rect(m_rows, kRowsLeft, bottom, kRowsRight - kRowsLeft, 24.f,
                 index % 2 ? ccColor4B{19, 29, 51, 225} : ccColor4B{28, 40, 66, 240});
            rect(m_rows, kRowsLeft, bottom, 3.f, 24.f, m_tab == 0 ?
                 ccColor4B{79, 163, 255, 255} : ccColor4B{161, 117, 255, 255});
            label(m_rows, fmt::format("#{}", lv.rank), kRowsLeft + 12.f, bottom + 12.f,
                  0.47f, m_tab == 0 ? ccColor3B{120, 202, 255} : ccColor3B{196, 169, 255}, 40.f);
            label(m_rows, lv.name, kRowsLeft + 62.f, bottom + 12.f,
                  0.43f, {235, 242, 255}, 249.f);
            label(m_rows, "by " + lv.creator, kRowsRight - 95.f, bottom + 12.f,
                  0.31f, {157, 174, 200}, 95.f);
            auto rowTap = CCMenuItemSpriteExtra::create(
                CCLayerColor::create({0, 0, 0, 0}, kRowsRight - kRowsLeft, 23.f),
                this, menu_selector(TSLPopup::onRow));
            rowTap->setTag(static_cast<int>(idx));
            rowTap->setPosition({(kRowsLeft + kRowsRight) * 0.5f, bottom + 12.f});
            rowsMenu->addChild(rowTap);
        }
    }

    void changeTab(int newTab) {
        if (newTab == m_tab) return;
        m_tab = newTab;
        m_page = 0;
        m_tabHighlight->setPositionX(m_tab == 0 ? 20.f : 242.f);
        m_heading->setString(m_tab == 0 ? "THE SHITTY LIST" : "THE SHITTY LIST PLUS");
        m_search.clear();
        if (m_searchInput) m_searchInput->setString("");
        setStatus(cache().live[m_tab] ? "live rankings loaded" : "offline sample - updating...");
        rebuild();
        if (!cache().live[m_tab]) load();
    }

    void load() {
        ++m_generation;
        int generation = m_generation;
        int tab = m_tab;
        m_loading = true;
        setStatus("fetching live rankings...");
        auto req = web::WebRequest();
        req.userAgent("TSLIntegration/0.1 (Geode; GD)");
        m_request.spawn(req.get(kUrls[tab]), [this, generation, tab](web::WebResponse response) {
            if (generation != m_generation || tab != m_tab) return;
            m_loading = false;
            if (!response.ok()) {
                setStatus(cache().live[tab] ? "refresh failed - showing cached list" : "offline preview - site unavailable",
                          {255, 172, 122});
                return;
            }
            auto body = response.string();
            if (body.isErr()) {
                setStatus("offline preview - response unreadable", {255, 172, 122});
                return;
            }
            auto parsed = tsl::parseListHTML(body.unwrap());
            if (parsed.empty()) {
                setStatus("offline preview - website layout changed", {255, 172, 122});
                return;
            }
            cache().levels[tab] = std::move(parsed);
            cache().live[tab] = true;
            m_page = 0;
            setStatus(fmt::format("live - {} levels synced", cache().levels[tab].size()), {127, 225, 192});
            rebuild();
        });
    }

    void onTabMain(CCObject*) { changeTab(0); }
    void onTabPlus(CCObject*) { changeTab(1); }
    void onRefresh(CCObject*) { load(); }
    void onPageBack(CCObject*) { if (m_page > 0) { --m_page; rebuild(); } }
    void onPageNext(CCObject*) { ++m_page; rebuild(); }
    void onWebsite(CCObject*) { web::openLinkInBrowser(kUrls[m_tab]); }
    void onRow(CCObject* sender) {
        auto index = static_cast<size_t>(sender->getTag());
        if (index >= cache().levels[m_tab].size()) return;
        auto const& lv = cache().levels[m_tab][index];
        auto message = fmt::format("<cy>#{} {}</c>\n<cg>Creator:</c> {}\n\nSearch this name in GD's online levels.\nUse [WEB] for the list page.",
            lv.rank, lv.name, lv.creator);
        FLAlertLayer::create("Level details", message.c_str(), "OK")->show();
    }

    bool setup() {
        if (!Popup::init(kWidth, kHeight, "GJ_square05.png")) return false;
        // A self-contained theme using Cocos color panels: no copied web assets.
        rect(m_mainLayer, 8.f, 8.f, kWidth - 16.f, kHeight - 16.f, {10, 18, 34, 247});
        rect(m_mainLayer, 8.f, kHeight - 47.f, kWidth - 16.f, 39.f, {19, 33, 58, 255});
        rect(m_mainLayer, 8.f, kHeight - 49.f, kWidth - 16.f, 2.f, {65, 127, 242, 225});
        label(m_mainLayer, "TSL", 20.f, kHeight - 29.f, 0.84f, {234, 243, 255});
        label(m_mainLayer, "INTEGRATION", 80.f, kHeight - 29.f, 0.43f, {133, 166, 212});
        label(m_mainLayer, "community demon rankings", 300.f, kHeight - 30.f,
              0.35f, {146, 174, 207}, 155.f);

        // Tabs + accent underline.
        rect(m_mainLayer, 18.f, 222.f, 444.f, 29.f, {21, 32, 53, 255});
        m_tabHighlight = rect(m_mainLayer, 20.f, 222.f, 218.f, 3.f, {88, 167, 255, 255});
        auto tabs = CCMenu::create();
        tabs->setPosition({0.f, 0.f});
        m_mainLayer->addChild(tabs);
        auto main = button(tabs, this, menu_selector(TSLPopup::onTabMain), "THE SHITTY LIST", 129.f, 238.f, 208.f);
        auto plus = button(tabs, this, menu_selector(TSLPopup::onTabPlus), "THE SHITTY LIST PLUS", 351.f, 238.f, 208.f,
                           {203, 174, 255});
        (void)main;
        (void)plus;
        // Active list title doubles as a connection/status cue.
        m_heading = label(m_mainLayer, "THE SHITTY LIST", 20.f, 207.f, 0.5f, {231, 241, 255});
        m_searchInput = TextInput::create(141.f, "search levels...", "chatFont.fnt");
        m_searchInput->setPosition({383.f, 205.f});
        m_searchInput->setScale(0.87f);
        m_searchInput->setCallback([this](std::string const& query) {
            m_search = query;
            m_page = 0;
            rebuild();
        });
        m_mainLayer->addChild(m_searchInput);

        m_rows = CCNode::create();
        m_rows->setPosition({0.f, 0.f});
        m_mainLayer->addChild(m_rows);
        rect(m_mainLayer, 18.f, 18.f, 444.f, 28.f, {20, 32, 54, 253});
        m_menu = CCMenu::create();
        m_menu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(m_menu);
        button(m_menu, this, menu_selector(TSLPopup::onPageBack), "<", 33.f, 33.f, 28.f);
        button(m_menu, this, menu_selector(TSLPopup::onPageNext), ">", 68.f, 33.f, 28.f);
        button(m_menu, this, menu_selector(TSLPopup::onRefresh), "REFRESH", 359.f, 33.f, 87.f, {132, 218, 252});
        button(m_menu, this, menu_selector(TSLPopup::onWebsite), "WEB", 432.f, 33.f, 51.f, {183, 159, 255});
        m_counter = label(m_mainLayer, "1/1", 99.f, 32.f, 0.35f, {185, 209, 244}, 142.f);
        m_status = label(m_mainLayer, "offline sample - updating...", 21.f, 12.f, 0.31f,
                         {144, 174, 209}, 360.f);
        rebuild();
        load();
        return true;
    }

public:
    static TSLPopup* create() {
        auto popup = new TSLPopup();
        if (popup && popup->setup()) { popup->autorelease(); return popup; }
        delete popup;
        return nullptr;
    }
};
} // namespace

class $modify(TSLMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        auto menu = this->getChildByID("bottom-menu");
        if (!menu) { preloadLists(); return true; }
        auto sprite = ButtonSprite::create("TSL", 55, true, "bigFont.fnt", "GJ_button_04.png", 26.f, 0.55f);
        auto item = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(TSLMenuLayer::openTSL));
        item->setID("tsl-integration-button");
        menu->addChild(item);
        menu->updateLayout();
        preloadLists();
        return true;
    }

    void openTSL(CCObject*) {
        if (auto popup = TSLPopup::create()) popup->show();
    }
};


// Matches the behavior of rank integrations: when you view a listed GD level,
// show a compact TSL / TSL+ ranking badge on the level information screen.
// Only the live data qualifies; the offline preview is deliberately not used.
class $modify(TSLRankLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;
        if (!level) return true;
        auto levelName = tsl::lowercase(std::string(level->m_levelName.c_str()));
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        int row = 0;
        for (int tab = 0; tab < 2; ++tab) {
            if (!cache().live[tab]) continue;
            auto const& entries = cache().levels[tab];
            auto it = std::find_if(entries.begin(), entries.end(), [&levelName](tsl::Level const& v) {
                return tsl::lowercase(v.name) == levelName;
            });
            if (it == entries.end()) continue;
            auto color = tab == 0 ? ccColor4B{64, 147, 244, 240} : ccColor4B{138, 102, 237, 240};
            auto dark = CCLayerColor::create({14, 23, 41, 235}, 121.f, 22.f);
            dark->setPosition({winSize.width - 130.f, winSize.height - 51.f - row * 26.f});
            this->addChild(dark, 15);
            auto stripe = CCLayerColor::create(color, 4.f, 22.f);
            stripe->setPosition({0.f, 0.f});
            dark->addChild(stripe);
            label(dark, fmt::format("{} #{}", tab == 0 ? "TSL" : "TSL+", it->rank),
                  11.f, 11.f, 0.47f, {229, 239, 255}, 106.f);
            ++row;
        }
        return true;
    }
};
