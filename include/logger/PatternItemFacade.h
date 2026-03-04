#pragma once

#include "LogEvent.h"
#include "common/alias.h"   // 自定义print
#include <iostream>
#include <ostream>
// #include <print>     // GCC 13不支持
#include <format>


/*=================================================PatternItemFacade==========================================*/

class PatternItemFacade{
protected:
    auto operator=(const PatternItemFacade&) -> PatternItemFacade& = delete;
    auto operator=(PatternItemFacade&&) -> PatternItemFacade& = delete;

public:
    PatternItemFacade() = default;
    PatternItemFacade(const PatternItemFacade&) = default;
    PatternItemFacade(PatternItemFacade&&) = default;

    virtual auto format(std::ostream& os, const LogEvent& event) -> size_t = 0;

    virtual ~PatternItemFacade() = default;
};
