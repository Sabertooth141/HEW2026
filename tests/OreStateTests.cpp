#include "../Framework/OreState.h"

#include <cstdint>
#include <iostream>
#include <limits>

namespace
{
int checks = 0;
int failures = 0;
const char* currentTest = "";

// Do not use assert: these checks must also execute with /DNDEBUG.
void Check(bool passed, const char* expression, int line)
{
    ++checks;
    if (passed) return;
    ++failures;
    std::cerr << "[FAIL] " << currentTest << ':' << line
              << " -- " << expression << '\n';
}

#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)

void DefaultTypes()
{
    // These are the current provisional balance values, not final game tuning.
    struct DefaultCase
    {
        OreType type;
        int hp;
        int collisionDamage;
        int experience;
    };
    constexpr DefaultCase defaults[] = {
        { OreType::Gray, 30, 5, 5 },
        { OreType::LightBlue, 60, 10, 10 },
        { OreType::Purple, 100, 20, 20 }
    };

    for (const auto& expected : defaults)
    {
        const OreState ore(expected.type);
        CHECK(ore.GetOreType() == expected.type);
        CHECK(ore.GetHP() == expected.hp);
        CHECK(ore.GetMaxHP() == expected.hp);
        CHECK(ore.GetStats().maxHp == expected.hp);
        CHECK(ore.GetStats().collisionDamage == expected.collisionDamage);
        CHECK(ore.GetStats().experience == expected.experience);
        CHECK(!ore.IsDestroyed());
    }

    const OreState implicitGray;
    CHECK(implicitGray.GetOreType() == OreType::Gray);
    CHECK(implicitGray.GetHP() == 30);
}

void CustomStatsAndDamage()
{
    OreState ore(OreType::LightBlue, { 17, 3, 9 });
    CHECK(ore.GetOreType() == OreType::LightBlue);
    CHECK(ore.GetHP() == 17);
    CHECK(ore.GetMaxHP() == 17);
    CHECK(ore.GetStats().collisionDamage == 3);
    CHECK(ore.GetStats().experience == 9);

    CHECK(ore.TakeDamage(7));
    CHECK(ore.GetHP() == 10);
    CHECK(!ore.IsDestroyed());
    CHECK(ore.TakeDamage(6));
    CHECK(ore.GetHP() == 4);
    CHECK(ore.GetMaxHP() == 17);
    CHECK(ore.GetStats().maxHp == 17);
    CHECK(ore.GetStats().collisionDamage == 3);
    CHECK(ore.GetStats().experience == 9);
}

void NonPositiveDamage()
{
    OreState ore(OreType::Gray, { 17, 3, 9 });
    constexpr int invalidDamage[] = { 0, -1, (std::numeric_limits<int>::min)() };
    for (int damage : invalidDamage)
    {
        CHECK(!ore.TakeDamage(damage));
        CHECK(ore.GetHP() == 17);
        CHECK(!ore.IsDestroyed());
    }
}

void LethalAndRepeatedDamage()
{
    OreState ore(OreType::Purple, { 17, 3, 9 });
    CHECK(ore.TakeDamage(17));
    CHECK(ore.GetHP() == 0);
    CHECK(ore.IsDestroyed());
    CHECK(ore.GetMaxHP() == 17);

    constexpr int subsequentDamage[] = {
        1, 0, -1, (std::numeric_limits<int>::max)(),
        (std::numeric_limits<int>::min)()
    };
    for (int damage : subsequentDamage)
    {
        CHECK(!ore.TakeDamage(damage));
        CHECK(ore.GetHP() == 0);
        CHECK(ore.IsDestroyed());
    }
}

void IntegerLimitsAndOverkill()
{
    constexpr int maxInt = (std::numeric_limits<int>::max)();
    OreState overkill(OreType::Gray, { 7, 0, 0 });
    CHECK(overkill.TakeDamage(maxInt));
    CHECK(overkill.GetHP() == 0);
    CHECK(overkill.IsDestroyed());
    CHECK(overkill.GetMaxHP() == 7);

    OreState maximumHp(OreType::Purple, { maxInt, maxInt, maxInt });
    CHECK(maximumHp.GetHP() == maxInt);
    CHECK(maximumHp.GetStats().collisionDamage == maxInt);
    CHECK(maximumHp.GetStats().experience == maxInt);
    CHECK(maximumHp.TakeDamage(maxInt - 1));
    CHECK(maximumHp.GetHP() == 1);
    CHECK(!maximumHp.IsDestroyed());
    CHECK(maximumHp.TakeDamage(1));
    CHECK(maximumHp.GetHP() == 0);
    CHECK(maximumHp.IsDestroyed());
    CHECK(maximumHp.GetMaxHP() == maxInt);
}

void InvalidStatsAreSafe()
{
    struct InvalidCase
    {
        OreStats input;
        OreStats expected;
    };
    constexpr int minInt = (std::numeric_limits<int>::min)();
    constexpr InvalidCase cases[] = {
        { { 0, 0, 0 }, { 1, 0, 0 } },
        { { -8, -3, -9 }, { 1, 0, 0 } },
        { { minInt, minInt, minInt }, { 1, 0, 0 } },
        { { 0, 4, 6 }, { 1, 4, 6 } },
        { { 23, -3, 6 }, { 23, 0, 6 } },
        { { 23, 4, -9 }, { 23, 4, 0 } }
    };

    for (const auto& test : cases)
    {
        OreState ore(OreType::Purple, test.input);
        CHECK(ore.GetOreType() == OreType::Purple);
        CHECK(ore.GetHP() == test.expected.maxHp);
        CHECK(ore.GetMaxHP() == test.expected.maxHp);
        CHECK(ore.GetStats().collisionDamage == test.expected.collisionDamage);
        CHECK(ore.GetStats().experience == test.expected.experience);
        CHECK(!ore.IsDestroyed());
        CHECK(ore.TakeDamage(test.expected.maxHp));
        CHECK(ore.GetHP() == 0);
        CHECK(ore.IsDestroyed());
    }
}

void RepeatedAndInterleavedSwings()
{
    OreState ore;
    const int initialHp = ore.GetHP();
    constexpr std::uint64_t firstHammerSwing = 101;
    constexpr std::uint64_t secondHammerSwing = 202;

    CHECK(!ore.TryRegisterHammerHit(0));
    CHECK(!ore.TryRegisterHammerHit(0));
    CHECK(ore.TryRegisterHammerHit(firstHammerSwing));
    CHECK(!ore.TryRegisterHammerHit(firstHammerSwing)); // Stay event.
    CHECK(ore.TryRegisterHammerHit(secondHammerSwing));
    CHECK(!ore.TryRegisterHammerHit(firstHammerSwing)); // Re-entry after another hammer.
    CHECK(!ore.TryRegisterHammerHit(secondHammerSwing));
    CHECK(!ore.TryRegisterHammerHit(firstHammerSwing));
    CHECK(ore.TryRegisterHammerHit(303)); // A genuinely new swing is accepted.
    CHECK(!ore.TryRegisterHammerHit(firstHammerSwing));
    CHECK(ore.TryRegisterHammerHit((std::numeric_limits<std::uint64_t>::max)()));
    CHECK(!ore.TryRegisterHammerHit((std::numeric_limits<std::uint64_t>::max)()));
    CHECK(ore.GetHP() == initialHp); // Registration alone must not apply damage.
    CHECK(!ore.IsDestroyed());
}

void SwingIdsArePerOre()
{
    OreState firstOre;
    OreState secondOre;
    CHECK(firstOre.TryRegisterHammerHit(42));
    CHECK(secondOre.TryRegisterHammerHit(42));
    CHECK(!firstOre.TryRegisterHammerHit(42));
    CHECK(!secondOre.TryRegisterHammerHit(42));
}

void DestroyedOreRejectsContacts()
{
    OreState ore(OreType::Gray, { 1, 5, 5 });
    CHECK(ore.TryRegisterHammerHit(101));
    CHECK(ore.TakeDamage(1));
    CHECK(!ore.TryRegisterHammerHit(101));
    CHECK(!ore.TryRegisterHammerHit(202));
    CHECK(!ore.TryRegisterHammerHit(0));
    CHECK(ore.GetHP() == 0);
    CHECK(ore.IsDestroyed());
}

void HammerStateContactDoesNotRequireSwing()
{
    OreState ore;
    constexpr std::uint64_t enemy = 10;
    CHECK(!ore.TryRegisterHammerContactDamage(0, 1, 0, 1));
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 0, 0, 1));
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 0, 0, 2));
    CHECK(ore.TryRegisterHammerContactDamage(enemy, 1, 0, 3));
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 1, 0, 3));
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 1, 0, 4));
    CHECK(ore.GetHP() == 30); // Contact costs enemy HP, not ore HP.
}

void ContactAndSwingShareOneEnemyDamageCost()
{
    OreState ore;
    constexpr std::uint64_t enemy = 10;
    CHECK(ore.TryRegisterHammerContactDamage(enemy, 1, 101, 1));
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 1, 101, 1));
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 1, 101, 2));
    CHECK(ore.TryRegisterHammerHit(101));
    CHECK(!ore.TryRegisterHammerHit(101));
    CHECK(ore.TryRegisterHammerContactDamage(enemy, 1, 102, 3));
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 1, 102, 3));
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 1, 0, 4)); // EndSwing.
    CHECK(ore.TryRegisterHammerContactDamage(enemy, 1, 103, 5));
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 1, 103, 6));
}

void StateSwitchInsideExistingContact()
{
    OreState ore;
    constexpr std::uint64_t enemy = 10;
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 0, 0, 1));
    CHECK(ore.TryRegisterHammerContactDamage(enemy, 1, 0, 2));
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 1, 0, 3));
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 0, 0, 4));
    CHECK(ore.TryRegisterHammerContactDamage(enemy, 2, 0, 5));
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 2, 0, 6));
    // A normal->hammer toggle between physics events still has a new token.
    CHECK(ore.TryRegisterHammerContactDamage(enemy, 3, 0, 7));
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 3, 0, 8));
}

void SameFrameDoubleCostsArePrevented()
{
    OreState ore;
    constexpr std::uint64_t enemy = 10;
    CHECK(ore.TryRegisterHammerContactDamage(enemy, 1, 0, 1));
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 1, 101, 1));
    // A new swing is not forgotten merely because first contact happened in
    // the same frame; the next Stay charges it once.
    CHECK(ore.TryRegisterHammerContactDamage(enemy, 1, 101, 2));
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 1, 101, 2));
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 1, 101, 3));

    ore.EndHammerContact(enemy);
    CHECK(ore.TryRegisterHammerContactDamage(enemy, 1, 101, 4));
    ore.EndHammerContact(enemy);
    CHECK(!ore.TryRegisterHammerContactDamage(enemy, 1, 101, 4));
    CHECK(ore.TryRegisterHammerContactDamage(enemy, 1, 101, 5));
    CHECK(!ore.TryRegisterHammerHit(0));
    CHECK(ore.TryRegisterHammerHit(101));
    ore.EndHammerContact(enemy);
    CHECK(!ore.TryRegisterHammerHit(101)); // Re-entry cannot repeat ore damage.
}

void ContactsAreIndependentAndStopAfterDestruction()
{
    OreState ore;
    CHECK(ore.TryRegisterHammerContactDamage(10, 1, 0, 1));
    CHECK(ore.TryRegisterHammerContactDamage(20, 1, 0, 1));
    CHECK(!ore.TryRegisterHammerContactDamage(10, 1, 0, 1));
    CHECK(!ore.TryRegisterHammerContactDamage(20, 1, 0, 1));
    CHECK(ore.TakeDamage(30));
    CHECK(!ore.TryRegisterHammerContactDamage(30, 1, 0, 2));
    CHECK(!ore.TryRegisterHammerContactDamage(10, 2, 102, 2));
}
}

int main()
{
    struct TestCase
    {
        const char* name;
        void (*run)();
    };
    const TestCase tests[] = {
        { "three default ore types", DefaultTypes },
        { "custom stats and ordinary damage", CustomStatsAndDamage },
        { "zero and negative damage", NonPositiveDamage },
        { "lethal and repeated damage", LethalAndRepeatedDamage },
        { "integer limits and overkill", IntegerLimitsAndOverkill },
        { "invalid stats normalization", InvalidStatsAreSafe },
        { "repeated and interleaved hammer swings", RepeatedAndInterleavedSwings },
        { "swing IDs are per ore", SwingIdsArePerOre },
        { "destroyed ore rejects contacts", DestroyedOreRejectsContacts },
        { "hammer state contact without a swing", HammerStateContactDoesNotRequireSwing },
        { "contact and swing share one enemy damage cost", ContactAndSwingShareOneEnemyDamageCost },
        { "state changes while contact continues", StateSwitchInsideExistingContact },
        { "same-frame double contact costs", SameFrameDoubleCostsArePrevented },
        { "independent contacts and destroyed ore", ContactsAreIndependentAndStopAfterDestruction }
    };

    for (const auto& test : tests)
    {
        currentTest = test.name;
        const int previousFailures = failures;
        test.run();
        if (failures == previousFailures)
            std::cout << "[PASS] " << test.name << '\n';
    }

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
