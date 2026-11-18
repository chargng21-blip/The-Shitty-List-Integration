#include <Geode/Geode.hpp>
#include <Geode/modify/LevelSearchLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/web.hpp>
#include "ListParser.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

using namespace geode::prelude;
namespace web = geode::utils::web;

namespace {
constexpr float W = 442.f;
constexpr float H = 286.f;
constexpr int kRows = 5;
constexpr std::array<char const*, 2> kUrls = {
    "https://tsl.pages.dev/list/tsl", "https://tsl.pages.dev/list/tslplus"
};

// These are only placeholders for when the site cannot be reached, never live ranks.
std::vector<tsl::Level> sample(int tab) {
    if (tab == 0) return {
        {1, "Shitty Acheron (Twoft)", "Twoft"}, {2, "Shitty Nhelv", "jackyy"},
        {3, "Shitty Discretion Advised", "Keruo"}, {4, "Shitty SARYYX NEVER CLEAR", "Twoft"},
        {5, "Shitty Sky Shredder", "phantom"}
    };
    return {
        {1, "Shitty Flamewall", "Deceit"}, {2, "Shitty Nullscapes", "dofar"},
        {3, "Shitty LIMBO", "Asteria."}, {4, "Shitty Kyouki", "Deceit"},
        {5, "Shitty Injury", "gzarvati"}
    };
}

struct ListCache {
    std::array<std::vector<tsl::Level>, 2> entries{sample(0), sample(1)};
    std::array<bool, 2> live{false, false};
};
ListCache& cache() {
    static ListCache c;
    return c;
}

CCLayerColor* panel(CCNode* parent, float x, float y, float w, float h, ccColor4B color) {
    auto p = CCLayerColor::create(color, w, h);
    p->setPosition({x, y});
    parent->addChild(p);
    return p;
}
CCLabelBMFont* text(CCNode* parent, std::string const& value, float x, float y,
                    float scale = .45f, ccColor3B color = {232, 239, 255},
                    float maxWidth = 0.f, bool centered = false) {
    auto t = CCLabelBMFont::create(value.c_str(), "bigFont.fnt");
    t->setColor(color);
    t->setScale(scale);
    t->setAnchorPoint(centered ? CCPoint{.5f, .5f} : CCPoint{0.f, .5f});
    t->setPosition({x,y});
    if (maxWidth > 0) t->limitLabelWidth(maxWidth, scale, .13f);
    parent->addChild(t);
    return t;
}
CCMenuItemSpriteExtra* addButton(CCMenu* menu, CCObject* target, SEL_MenuHandler action,
                                 std::string const& caption, float x, float y, float w,
                                 ccColor4B bg = {35, 53, 88, 255},
                                 ccColor3B fg = {225, 239, 255}) {
    auto surface = CCLayerColor::create(bg, w, 23.f);
    text(surface, caption, w/2.f, 11.5f, .40f, fg, w - 7.f, true);
    auto btn = CCMenuItemSpriteExtra::create(surface, target, action);
    btn->setPosition({x,y});
    menu->addChild(btn);
    return btn;
}

// Use GD's own online level browser so rows lead to actual playable level entries.
// Searching by a verified ID is exact. The TSL web list does not expose stable
// IDs in its visible card text, so name search is the safe fallback.
void openLevelInGD(tsl::Level const& level) {
    auto query = level.id > 0 ? std::to_string(level.id) : level.name;
    auto search = GJSearchObject::create(SearchType::Search, query);
    auto scene = LevelBrowserLayer::scene(search);
    CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(.20f, scene));
}

class TSLPopup final : public geode::Popup {
    int m_tab = 0;
    int m_page = 0;
    int m_generation = 0;
    bool m_busy = false;
    std::string m_query;
    std::vector<size_t> m_filtered;
    CCNode* m_rowHost = nullptr;
    CCLayerColor* m_activeUnderline = nullptr;
    CCLabelBMFont* m_pageLabel = nullptr;
    CCLabelBMFont* m_statusLabel = nullptr;
    CCLabelBMFont* m_titleLabel = nullptr;
    TextInput* m_search = nullptr;
    geode::async::TaskHolder<web::WebResponse> m_request;

    void status(std::string const& s, ccColor3B color = {145, 176, 210}) {
        if (m_statusLabel) {
            m_statusLabel->setString(s.c_str());
            m_statusLabel->setColor(color);
        }
    }
    void render() {
        if (!m_rowHost || !m_pageLabel) return;
        m_rowHost->removeAllChildrenWithCleanup(true);
        m_filtered.clear();
        auto const& levels = cache().entries[m_tab];
        auto q = tsl::lowercase(m_query);
        for (size_t i = 0; i < levels.size(); ++i) {
            auto const& lv = levels[i];
            auto hay = tsl::lowercase(lv.name + " " + lv.creator + " " + std::to_string(lv.rank));
            if (q.empty() || hay.find(q) != std::string::npos) m_filtered.push_back(i);
        }
        int pages = std::max(1, static_cast<int>((m_filtered.size() + kRows - 1) / kRows));
        m_page = std::clamp(m_page, 0, pages - 1);
        m_pageLabel->setString(fmt::format("PAGE {} / {}  |  {} LEVELS", m_page+1, pages, m_filtered.size()).c_str());
        if (m_filtered.empty()) {
            text(m_rowHost, "NO MATCHING LEVELS", W/2.f, 111.f, .57f, {152, 174, 207}, 290.f, true);
            return;
        }
        auto menu = CCMenu::create();
        menu->setPosition({0,0});
        m_rowHost->addChild(menu, 2);
        for (int i = 0; i < kRows && static_cast<size_t>(m_page * kRows + i) < m_filtered.size(); ++i) {
            size_t idx = m_filtered[m_page*kRows + i];
            auto const& lv = levels[idx];
            // Five 27-pixel rows; each has its OWN click target, never overlaid by another row.
            float bottom = 165.f - i * 28.f;
            panel(m_rowHost, 17.f, bottom, 408.f, 26.f, i%2 ?
                ccColor4B{21,33,55,252} : ccColor4B{28,43,70,252});
            panel(m_rowHost, 17.f, bottom, 3.f, 26.f, m_tab == 0 ?
                ccColor4B{85,181,252,255} : ccColor4B{182,131,253,255});
            text(m_rowHost, fmt::format("#{}", lv.rank), 27.f, bottom+13.f, .43f,
                m_tab == 0 ? ccColor3B{122,208,251} : ccColor3B{207,176,255}, 41.f);
            text(m_rowHost, lv.name, 76.f, bottom+13.f, .40f, {239,245,255}, 236.f);
            text(m_rowHost, "by " + lv.creator, 318.f, bottom+13.f, .29f, {155,175,211}, 90.f);
            text(m_rowHost, ">", 412.f, bottom+13.f, .43f, {163,213,249}, 12.f);
            auto hitbox = CCLayerColor::create({0,0,0,0}, 408.f, 26.f);
            auto click = CCMenuItemSpriteExtra::create(hitbox, this, menu_selector(TSLPopup::onOpenLevel));
            click->setTag(static_cast<int>(idx));
            click->setPosition({W/2.f, bottom+13.f});
            menu->addChild(click);
        }
    }
    void selectTab(int tab) {
        if (tab == m_tab) return;
        ++m_generation; // invalidate any callback from the previous tab
        m_busy = false;
        m_tab = tab;
        m_page = 0;
        m_query.clear();
        if (m_search) m_search->setString("");
        m_activeUnderline->setPositionX(tab == 0 ? 18.f : 223.f);
        m_activeUnderline->setColor(tab == 0 ? ccColor3B{83,180,255} : ccColor3B{184,137,250});
        m_titleLabel->setString(tab == 0 ? "THE SHITTY LIST" : "THE SHITTY LIST PLUS");
        status(cache().live[tab] ? "live ranking data" : "sample only | refreshing...");
        render();
        if (!cache().live[tab]) refresh();
    }
    void refresh() {
        if (m_busy) return;
        ++m_generation;
        auto generation = m_generation;
        auto tab = m_tab;
        m_busy = true;
        status("loading list from tsl.pages.dev...");
        auto request = web::WebRequest();
        request.userAgent("TSLIntegration/0.2 (Geode)");
        m_request.spawn(request.get(kUrls[tab]), [this, generation, tab](web::WebResponse response) {
            if (generation != m_generation || tab != m_tab) return;
            m_busy = false;
            if (!response.ok()) {
                status(cache().live[tab] ? "refresh failed | cached rankings" :
                    "offline | sample rankings only", {252, 175, 132});
                return;
            }
            auto data = response.string();
            if (data.isErr()) {
                status("could not read the list response", {252, 175, 132});
                return;
            }
            auto levels = tsl::parseListHTML(data.unwrap());
            if (levels.empty()) {
                status("website layout changed | sample shown", {252, 175, 132});
                return;
            }
            cache().entries[tab] = std::move(levels);
            cache().live[tab] = true;
            m_page = 0;
            status(fmt::format("synced {} live levels | select to play", cache().entries[tab].size()),
                   {123, 224, 192});
            render();
        });
    }
    void onOpenLevel(CCObject* sender) {
        auto idx = static_cast<size_t>(sender->getTag());
        auto const& v = cache().entries[m_tab];
        if (idx >= v.size()) return;
        auto choice = v[idx];
        // Close the overlay before switching to a native GD scene.
        onClose(nullptr);
        openLevelInGD(choice);
    }
    void onTabMain(CCObject*) { selectTab(0); }
    void onTabPlus(CCObject*) { selectTab(1); }
    void onPrevious(CCObject*) { if (m_page > 0) { --m_page; render(); } }
    void onNext(CCObject*) { ++m_page; render(); }
    void onReload(CCObject*) { refresh(); }
    void onWeb(CCObject*) { web::openLinkInBrowser(kUrls[m_tab]); }

    bool setup() {
        if (!Popup::init(W, H, "GJ_square05.png")) return false;
        // Everything is relative to the popup's CONTENT area; buttons and rows
        // share the same unscaled coordinate system, avoiding the old overlap bug.
        panel(m_mainLayer, 8.f, 8.f, W-16.f, H-16.f, {10,19,36,251});
        panel(m_mainLayer, 8.f, H-46.f, W-16.f, 38.f, {20,36,61,255});
        panel(m_mainLayer, 8.f, H-48.f, W-16.f, 2.f, {76,137,211,255});
        text(m_mainLayer, "TSL", 20.f, H-29.f, .78f, {232,247,255});
        text(m_mainLayer, "INTEGRATION", 79.f, H-29.f, .42f, {136,183,228});
        text(m_mainLayer, "PLAY LIST LEVELS IN GD", 279.f, H-29.f, .30f, {157,177,210});

        panel(m_mainLayer, 16.f, 213.f, 410.f, 27.f, {19,32,55,255});
        m_activeUnderline = panel(m_mainLayer, 18.f, 213.f, 200.f, 3.f, {83,180,255,255});
        auto top = CCMenu::create();
        top->setPosition({0,0});
        m_mainLayer->addChild(top, 3);
        addButton(top, this, menu_selector(TSLPopup::onTabMain), "THE SHITTY LIST", 118.f, 227.f, 197.f);
        addButton(top, this, menu_selector(TSLPopup::onTabPlus), "THE SHITTY LIST PLUS", 323.f, 227.f, 197.f,
                  {42,42,77,255}, {211,189,255});
        m_titleLabel = text(m_mainLayer, "THE SHITTY LIST", 19.f, 200.f, .45f, {232,240,255});
        m_search = TextInput::create(153.f, "search levels...", "chatFont.fnt");
        m_search->setPosition({346.f, 198.f});
        m_search->setScale(.85f);
        m_search->setCallback([this](std::string const& q) {
            m_query = q;
            m_page = 0;
            render();
        });
        m_mainLayer->addChild(m_search, 3);

        m_rowHost = CCNode::create();
        m_rowHost->setPosition({0,0});
        m_mainLayer->addChild(m_rowHost, 2);
        panel(m_mainLayer, 16.f, 11.f, 410.f, 33.f, {20,33,56,255});
        auto controls = CCMenu::create();
        controls->setPosition({0,0});
        m_mainLayer->addChild(controls, 3);
        addButton(controls, this, menu_selector(TSLPopup::onPrevious), "<", 33.f, 28.f, 28.f);
        addButton(controls, this, menu_selector(TSLPopup::onNext), ">", 65.f, 28.f, 28.f);
        addButton(controls, this, menu_selector(TSLPopup::onReload), "SYNC", 346.f, 28.f, 61.f,
                  {38,66,93,255}, {159,225,248});
        addButton(controls, this, menu_selector(TSLPopup::onWeb), "WEB", 405.f, 28.f, 45.f,
                  {57,46,90,255}, {214,186,255});
        m_pageLabel = text(m_mainLayer, "PAGE 1 / 1", 89.f, 31.f, .31f, {196,217,244}, 236.f);
        m_statusLabel = text(m_mainLayer, "sample only | refreshing...", 90.f, 15.f, .26f,
                             {142,174,207}, 224.f);
        render();
        refresh();
        return true;
    }
public:
    static TSLPopup* create() {
        auto p = new TSLPopup();
        if (p && p->setup()) { p->autorelease(); return p; }
        delete p;
        return nullptr;
    }
};
} // namespace

// Same ENTRY SCREEN as Integrated Demonlist: the in-game online LEVEL SEARCH
// screen (Create > Search), NOT the main menu. The TSL button sits near the
// right edge and does not reuse or modify Integrated Demonlist's own nodes.
class $modify(TSLLevelSearchLayer, LevelSearchLayer) {
    bool init() {
        if (!LevelSearchLayer::init()) return false;
        auto size = CCDirector::sharedDirector()->getWinSize();
        auto menu = CCMenu::create();
        menu->setPosition({0,0});
        menu->setID("tsl-level-search-menu");
        this->addChild(menu, 50);
        auto sprite = ButtonSprite::create("TSL", 56, true, "bigFont.fnt", "GJ_button_04.png", 28.f, .58f);
        auto item = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(TSLLevelSearchLayer::openTSL));
        item->setID("tsl-level-search-button");
        // Bottom-right of the search screen, adjacent to native search controls.
        item->setPosition({size.width - 43.f, 66.f});
        menu->addChild(item);
        return true;
    }
    void openTSL(CCObject*) {
        if (auto p = TSLPopup::create()) p->show();
    }
};

class $modify(TSLRankLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;
        if (!level) return true;
        auto levelName = tsl::lowercase(std::string(level->m_levelName.c_str()));
        auto id = static_cast<int>(level->m_levelID.value());
        auto screen = CCDirector::sharedDirector()->getWinSize();
        int row = 0;
        for (int tab = 0; tab < 2; ++tab) {
            if (!cache().live[tab]) continue;
            auto const& entries = cache().entries[tab];
            auto it = std::find_if(entries.begin(), entries.end(), [&](tsl::Level const& lv) {
                return lv.id > 0 ? (lv.id == id) : (tsl::lowercase(lv.name) == levelName);
            });
            if (it == entries.end()) continue;
            auto bg = CCLayerColor::create({15,26,45,240}, 116.f, 21.f);
            bg->setPosition({screen.width - 125.f, screen.height - 53.f - row * 25.f});
            this->addChild(bg, 15);
            panel(bg, 0.f, 0.f, 4.f, 21.f, tab ? ccColor4B{157,116,238,255} :
                  ccColor4B{66,160,244,255});
            text(bg, fmt::format("{} #{}", tab ? "TSL+" : "TSL", it->rank),
                 10.f, 10.f, .42f, {227,239,255}, 100.f);
            ++row;
        }
        return true;
    }
};
