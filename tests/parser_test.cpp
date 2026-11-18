#include "../src/ListParser.hpp"
#include <cassert>
#include <iostream>
int main(){
    auto a=tsl::parseListHTML(R"HTML(<html><head><script>const fake='#44 Shitty FAKE by Bot';</script></head><body>
      <section><div>#1</div><h2>Shitty Flamewall</h2><p>by Deceit</p></section>
      <section><div>#2</div><h2>Shitty Nullscapes</h2><p>by dofar</p></section>
      <section><div>#3</div><h2>Shitty LIMBO</h2><p>by Asteria.</p></section>
    </body></html>)HTML");
    assert(a.size()==3 && a[0].rank==1 && a[0].name=="Shitty Flamewall" && a[0].creator=="Deceit");
    assert(a[1].name=="Shitty Nullscapes");
    auto b=tsl::parseListText("#1 Shitty Acheron (Twoft) by Twoft #2 Shitty Nhelv by jackyy");
    assert(b.size()==2 && b[1].name=="Shitty Nhelv");
    auto c=tsl::parseListHTML("<div>#1 Shitty Foo &amp; Bar by [A$AP] Revalutionary</div><div>#2 Shitty Nhelv by jackyy</div>");
    assert(c.size()==2 && c[0].name=="Shitty Foo & Bar" && c[0].creator=="[A$AP] Revalutionary");
    std::cout << "parser tests passed\n";
}
