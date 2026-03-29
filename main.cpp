#include <SFML/Graphics.hpp>
#include <iostream>
#include <queue>

// game object for player to store all player attributes
struct Player
{
    sf::RectangleShape body{ { 25.f, 25.f } };
    float speed = 200.f;
    int cheeseCollected = 0;
    // check for if player manually exited or was caught
    bool survived;

};

struct Cat 
{
    sf::RectangleShape body{ { 25.f, 25.f } };
    // timer used to have cat stop searching
    float freezeTimer = 0.f;
};

// utilize game state to determine which screen to display
enum GameState
{
    PLAYING,
    GAMEOVER
};
GameState state = PLAYING;


// map layout utilized to construct walls and cheese balls
std::vector<std::string> mazeMap =
{
    ".......W........................",
    ".......W.......C...........C....",
    ".......W........................",
    ".......W...WWWWWWWWWW...WWWWW...",
    "........................W.......",
    "WWWW...........C........W..C....",
    "........................W.......",
    ".......WWWWWWWWWWWWWW...W...W...",
    "..C....W....................W...",
    ".......W..C.................W...",
    "WWWW...W....................W...",
    ".......W...W...WWWWW....W.......",
    "..C....W...W.......W....W.......",
    ".......W...W....C..W....W.......",
    "...WWWWW...W.......W....W...WWWW",
    "........................W.......",
    "........................W.......",
    "........................W.......",
    "...WWWWW....W....WWWWWWWW...W...",
    "...W........W....W..........W...",
    "...W..C.....W....W...C......W...",
    "...W........W....W..........W...",
    "...W....WWWWW....W......WWWWW...",
    "...........W....................",
    "...........W....................",
    "...........W...WWWWWWW...WWWW...",
    "...W..C....W...W.........W......",
    "...W.......W...W..C......W......",
    "...WWWWW...W...W.........W......",
    "...........W.............W......",
    ".....C.....W.............W......",
    "...........W.............W......",
};


// manhattan distance calculator for use in pathFinder
int manhattanDistance(int firstX, int firstY, int secondX, int secondY) {

    return abs(firstX - secondX) + abs(firstY - secondY);
    
}

// helper function for moving the cat to the next position
void catMovement(int catX, int catY, int goalX, int goalY, Cat& cat, float deltaTime) {
    // one of these should be 0 as we only move in one direction and the other should be positive or negative
    int distX = goalX - catX;
    int distY = goalY - catY;
    float catSpeed = 150.f;

    if (distX == 0) 
    {
        if (distY > 0) 
        {
            cat.body.move({ 0.f, catSpeed * deltaTime });
        }
        else 
        {
            cat.body.move({ 0.f, -catSpeed * deltaTime });
        }
    }
    else if (distY == 0) 
    {
        if (distX > 0)
        {
            cat.body.move({ catSpeed * deltaTime, 0.f});
        }
        else
        {
            cat.body.move({ -catSpeed * deltaTime, 0.f });
        }
    }
}

// helper function to find a path from the cat to the player
// this is a BFS algorithm copied and altered from online
void nextMove(int catX, int catY, int playerX, int playerY, int grid[32][32], Cat& cat, float deltaTime) {

    // setup distance for grid cells
    int distance[32][32];
    std::fill(&distance[0][0], &distance[0][0] + 32 * 32, -1);

    // setup queue for BFS
    std::queue<std::pair<int, int>> q;
    q.push({ playerX, playerY });
    distance[playerY][playerX] = 0;

    // directions for neighbors
    int directionX[4] = { 0,0,-1,1 };
    int directionY[4] = { -1,1,0,0 };

    // populates distance with current distance values to find optimal next choice
    while (!q.empty()) {
        auto [x, y] = q.front(); q.pop();
        for (int i = 0;i < 4;i++) {
            int newX = x + directionX[i], newY = y + directionY[i];
            // checks
            if (newX < 0 || newY < 0 || newX >= 32 || newY >= 32) 
                continue;
            if (grid[newY][newX] == 100) 
                continue;
            if (distance[newY][newX] != -1) 
                continue;
            distance[newY][newX] = distance[y][x] + 1;
            q.push({ newX,newY });
        }
    }

    // pick neighbor with optimal distance
    int bestDist = 1000;
    int bestX = catX, bestY = catY;
    for (int i = 0;i < 4;i++) {
        int nx = catX + directionX[i], ny = catY + directionY[i];
        // checks
        if (nx < 0 || ny < 0 || nx >= 32 || ny >= 32) 
            continue;
        if (grid[ny][nx] == 100) 
            continue;
        if (distance[ny][nx] != -1 && distance[ny][nx] < bestDist) {
            bestDist = distance[ny][nx];
            bestX = nx; bestY = ny;
        }
    }

    catMovement(catX, catY, bestX, bestY, cat, deltaTime);

}


// behavior for the cat
void catBehavior(Player& player, Cat& cat, int grid[32][32], float deltaTime)
{

    if (cat.freezeTimer <= 0.f) 
    {
        sf::Vector2 playerPosition = player.body.getPosition();
        sf::Vector2 catPosition = cat.body.getPosition();

        // call nextMove() using converted position values to align with the grid
        nextMove(int(catPosition.x / 25.f), int(catPosition.y / 25.f), int(playerPosition.x / 25.f), int(playerPosition.y / 25.f), grid, cat, deltaTime);

        if (cat.body.getGlobalBounds().findIntersection(player.body.getGlobalBounds()))
            state = GAMEOVER;

    }    
    
}

// helper function for tracking player movement and player interaction with the level
// returns score to keep it updated
void playerInteraction(float deltaTime, Player& player, Cat& cat, std::vector<sf::RectangleShape>& walls, std::vector<sf::CircleShape>& cheeseBalls, sf::RectangleShape exit)
{
    // store players position before moving
    sf::Vector2f playerLastPosition = player.body.getPosition();

    // basic player controls
    // using else if to force player to commit to a certain direction, no diagonal movement
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
        player.body.move({ 0.f, -player.speed * deltaTime });
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
        player.body.move({ 0.f, player.speed * deltaTime });
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        player.body.move({ -player.speed * deltaTime, 0.f });
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        player.body.move({ player.speed * deltaTime, 0.f });

    // extracting playerBounds as to not have to store it each iteration of walls
    sf::FloatRect playerBounds = player.body.getGlobalBounds();
    

    // player collision detection
    for (const auto& wall : walls) 
    {
        if (playerBounds.findIntersection(wall.getGlobalBounds()))
            player.body.setPosition(playerLastPosition);
    }

    // player pick up detection
    for (int i = 0; i < cheeseBalls.size(); i++)
    {
        if (playerBounds.findIntersection(cheeseBalls[i].getGlobalBounds())) {
            cheeseBalls.erase(cheeseBalls.begin() + i);
            player.cheeseCollected += 1;
            cat.freezeTimer = 0.25;
            
        }
    }

    // player attempting to leave 
    // first check if player is on the exit point
    if (playerBounds.findIntersection(exit.getGlobalBounds())) 
    {
        // if E is then also pressed, terminate the game
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)) 
        {
            player.survived = true;
            state = GAMEOVER;
        }
    }

}

// helper function for updating window with new changes
void updateWindow(sf::RenderWindow& window, Player& player, Cat& cat, std::vector<sf::RectangleShape>& walls, std::vector<sf::CircleShape>& cheeseBalls, sf::RectangleShape exit)
{
    window.clear();
    window.draw(exit);
    window.draw(player.body);
    window.draw(cat.body);

    for (const auto& wall : walls)
    {
        window.draw(wall);
    }

    for (auto& cheese : cheeseBalls)
    {
        window.draw(cheese);
    }

    window.display();
}

// helper function for creating the walls of the maze
void addWall(std::vector<sf::RectangleShape>& walls, sf::Vector2f size, sf::Vector2f position)
{
    sf::RectangleShape wall(size);
    wall.setPosition(position);
    wall.setFillColor(sf::Color(92, 60, 17));
    walls.push_back(wall);
}

// helper function for creating the cheese balls for the maze
void addCheese(std::vector<sf::CircleShape>& cheeseBalls, float size, sf::Vector2f position)
{
    sf::CircleShape cheese(size);
    cheese.setPosition(position);
    cheese.setFillColor(sf::Color::Yellow);
    cheeseBalls.push_back(cheese);
}

// helper function for creating the maze level
// returns a pair for two lists, one for the walls and one for the cheese balls
std::pair<std::vector<sf::RectangleShape>, std::vector<sf::CircleShape>> createMaze()
{
    std::vector<sf::RectangleShape> walls;
    std::vector<sf::CircleShape> cheeseBalls;

    // Game border walls
    addWall(walls, { 800.f, 20.f }, { 0.f, 0.f });
    addWall(walls, { 800.f, 20.f }, { 0.f, 780.f });
    addWall(walls, { 20.f, 800.f }, { 0.f, 0.f });
    addWall(walls, { 20.f, 800.f }, { 780.f, 0.f });

    float tileSize = 25.f;

    for (int x = 0; x < mazeMap.size(); x++) 
    {
        for (int y = 0; y < mazeMap[x].size(); y++) 
        {
            if (mazeMap[y][x] == 'W') 
            {
                addWall(walls, { tileSize, tileSize }, { x * tileSize, y * tileSize });
            }
            else if (mazeMap[y][x] == 'C')
            {
                addCheese(cheeseBalls, tileSize / 3, { x * tileSize, y * tileSize });
            }
            
        }
    }

    return {walls, cheeseBalls};
    
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(800, 800)), "Greedy Mouse");
    
    // Create player
    Player player;
    player.body.setFillColor(sf::Color::White);
    player.body.setPosition({ 25.f, 25.f });
    

    // Create cat
    Cat cat;
    cat.body.setFillColor(sf::Color::Magenta);
    cat.body.setPosition({ 400.f, 400.f });

    // Create grid used for the cat's chase logic
    int grid[32][32];
    for (int y = 0; y < mazeMap.size(); y++)
    {
        for (int x = 0; x < mazeMap[y].size(); x++)
        {
            if (mazeMap[y][x] == 'W')
            {
                grid[y][x] = 100;
            }
            else 
            {
                grid[y][x] = 1;
            }
        }
    }

    // Create walls
    std::vector<sf::RectangleShape> walls;
    walls = createMaze().first;

    // Create cheese balls
    std::vector<sf::CircleShape> cheeseBalls;
    cheeseBalls = createMaze().second;

    // Create the exit door, interact with E to leave at any time
    sf::RectangleShape exit({ 50.f, 50.f });
    exit.setPosition({ 690.f, 690.f });
    exit.setFillColor(sf::Color::Green);

    // set up clock used for deltaTime
    sf::Clock deltaTimeClock;

    // set up clock for total elapsedTime
    sf::Clock clock;

    

    // game loop
    while (window.isOpen()) 
    {
        // check if window closed manually
        while (auto event = window.pollEvent()) 
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        // level is currently active and player has not lost or exited
        if (state == PLAYING)
        {
            // set deltaTime on each loop
            float deltaTime = deltaTimeClock.restart().asSeconds();

            // if the cat is frozen, decrease timer
            if (cat.freezeTimer > 0.f) {
                cat.freezeTimer -= deltaTime;
            }

            playerInteraction(deltaTime, player,cat, walls, cheeseBalls, exit);
            catBehavior(player, cat, grid, deltaTime);
            updateWindow(window, player, cat, walls, cheeseBalls, exit);
        }

        // handle game ending
        else if (state == GAMEOVER) 
        {
            window.close();

            // calculate final score
            float finalScore = (player.cheeseCollected * 10000) / clock.getElapsedTime().asSeconds();

            std::cout << "GAME OVER \n" << std::endl;

            // player survived
            if (player.survived) {
                std::cout << "You escaped! \n" << std::endl;
                std::cout << "Your final score is: " << int(finalScore) << std::endl;
            }
            // player was caught
            else {
                std::cout << "You got caught! \n" << std::endl;
                std::cout << "Your final score is: " << int(finalScore / 10) << std::endl;
            }
                
        }

    }

    return 0;
}


