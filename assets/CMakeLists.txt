
file(GLOB_RECURSE embskin "binary/VectorTheme/*")

juce_add_binary_data(BinaryData SOURCES
    binary/icon.svg
    binary/icon-links.svg
    binary/logo.svg
    binary/Jersey20.ttf
    binary/Trek.ttf
    ${embskin}
)

