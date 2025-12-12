#include "GameEngine.h"
#include "TestScene.h"

int main()
{
    GameEngine game;

    std::shared_ptr<SceneTest> scene = std::make_shared<SceneTest>(&game);
    game.changeScene("TEST", scene);
    game.run();

    return 0;
}
