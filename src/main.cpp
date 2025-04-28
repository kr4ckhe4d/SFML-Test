// --- Includes ---
// This header is the main entry point for SFML's graphics module.
// It includes everything needed for window creation, 2D drawing (shapes, sprites, text),
// textures, views (cameras), and handling graphics-related events.
#include <SFML/Graphics.hpp>
// This header provides std::optional, a C++17 feature used by SFML 3's
// window.pollEvent() function. pollEvent might return an event, or it might
// return nothing (if the event queue is empty), hence the 'optional'.
#include <optional>
// This header provides std::vector, a dynamic array container. We use it
// to create the 2D grid (a vector of vectors) that represents our level map.
#include <vector>
// This header provides std::string for working with text, like converting
// numbers to strings for display (though not used in this specific version yet).
#include <string>
// This header provides standard input/output stream objects like std::cout (for
// printing debug messages to the console) and std::cerr (for error messages).
#include <iostream>
// This header provides common mathematical functions like std::sqrt (square root),
// used here for normalizing the movement vector.
#include <cmath>
// This header provides std::filesystem::path, which is required by SFML 3's
// file loading functions (like Font::openFromFile) for handling file paths
// in a cross-platform way.
#include <filesystem>
// This header provides various algorithm utilities, including std::clamp,
// used here to restrict the camera's view within the level boundaries.
#include <algorithm>

// --- Global Constants ---
// Using constants makes the code easier to read and modify. If we want to
// change gravity, we only need to change it in one place.

// Downward acceleration applied to the player each frame (pixels/frame^2).
// Simulates gravity pulling the player down.
const float GRAVITY = 0.8f;
// Horizontal speed when the left/right keys are held (pixels/frame).
const float PLAYER_MOVE_SPEED = 5.0f;
// Initial vertical velocity when the jump key is pressed (pixels/frame).
// Negative because SFML's Y-axis points downwards (0 is top, height is bottom).
const float PLAYER_JUMP_VELOCITY = -18.0f;
// The dimension (width and height) of a single square tile in pixels.
// This links the grid-based level data to the pixel-based screen coordinates.
const int TILE_SIZE = 40;
// A small value used to prevent floating-point inaccuracies during collision checks,
// especially when the player is exactly aligned with a tile edge. It helps avoid
// getting stuck by checking slightly *inside* the player's bounds.
const float COLLISION_EPSILON = 0.01f;
// Window dimensions defined as constants for clarity and easy reference,
// particularly when setting up the initial view size.
const unsigned int WINDOW_WIDTH = 800;
const unsigned int WINDOW_HEIGHT = 600;


// --- Level Representation ---

// Defines symbolic names for different types of tiles in the level grid.
// Using an enum improves code readability compared to using raw numbers like 0 or 1.
enum TileType {
    Air = 0,   // Represents empty space. Player can move through these tiles.
    Solid = 1  // Represents solid ground/walls. Player collides with these.
};

// Structure (`struct`) to group together all data related to a game level.
struct Level {
    // The core level data: a 2D grid represented by a vector of vectors.
    // `tiles[row][column]` or `tiles[y][x]` holds the TileType for that location.
    std::vector<std::vector<TileType>> tiles;
    // The dimensions of the level grid in number of tiles (e.g., 40 tiles wide).
    sf::Vector2u size; // sf::Vector2u holds two unsigned integers (x, y).
    // The dimensions of the level converted to pixels. Calculated once for efficiency.
    // Useful for boundary checks involving pixel coordinates (like the view).
    sf::Vector2f sizePixels; // sf::Vector2f holds two floats (x, y).

    // Member function to safely retrieve the tile type at given grid coordinates (x, y).
    // `const` indicates this function doesn't modify the Level object's data.
    TileType getTile(int x, int y) const {
        // Boundary check: Ensure x and y are within the valid range of the grid.
        if (x >= 0 && x < size.x && y >= 0 && y < size.y) {
            // Valid coordinates: return the tile type from the 2D vector.
            return tiles[y][x]; // Note: Access is typically [row][column] or [y][x].
        }
        // Invalid coordinates (outside the map): Treat as Air to prevent errors
        // and simplify collision logic near the level edges.
        return Air;
    }
};

// --- Player Representation ---

// Structure to group together data and functions for the player character.
struct Player {
    // The player's visual representation. Currently a simple rectangle.
    // This could be replaced with sf::Sprite to use images/animations.
    // sf::RectangleShape is a drawable SFML entity.
    sf::RectangleShape shape;
    // Player's current speed and direction (pixels per frame). {x, y} components.
    sf::Vector2f velocity = {0.f, 0.f};
    // Flag to track if the player is currently standing on a solid surface.
    // Used primarily to determine if the player can jump.
    bool isOnGround = false;

    // Constructor: Initializes a new Player object.
    // Takes the starting position (in pixels) as an argument.
    Player(sf::Vector2f startPos) {
        // Set the player rectangle's size, slightly smaller than a tile.
        shape.setSize({TILE_SIZE * 0.8f, TILE_SIZE * 0.95f});
        // Set the player's color.
        shape.setFillColor(sf::Color::Green);
        // Set the shape's origin (the point around which transformations like
        // setPosition and rotation occur) to its center. This simplifies positioning.
        shape.setOrigin(shape.getSize() / 2.f);
        // Place the player's origin at the specified starting position.
        shape.setPosition(startPos);
    }

    // Simulates gravity by modifying the player's vertical velocity.
    void applyGravity() {
        // Increase the downward velocity component (y) by the GRAVITY constant.
        velocity.y += GRAVITY;
    }

    // Makes the player jump if they are currently on the ground.
    void jump() {
        // Only allow jumping if the flag indicates the player is grounded.
        if (isOnGround) {
            // Set the vertical velocity to the predefined jump velocity (upwards).
            velocity.y = PLAYER_JUMP_VELOCITY;
            // Player is no longer on the ground after jumping.
            isOnGround = false;
        }
    }

    // Detects and resolves collisions between the player and solid level tiles.
    // This is a core part of the platformer physics engine.
    // Takes a constant reference to the level data to check against.
    void handleCollision(const Level& level) {
        // Assume the player is not on the ground at the start of the check.
        // It will be set to true only if a downward collision is confirmed.
        isOnGround = false;
        // Get the player's current world-coordinate bounding box.
        sf::FloatRect playerBounds = shape.getGlobalBounds();

        // --- Vertical Collision Check ---
        // Check collisions along the Y-axis first. Resolving vertical collisions
        // before horizontal ones often leads to more stable platformer physics.

        // Create a copy of the bounds to predict where the player *will be* vertically.
        sf::FloatRect verticalCheckBounds = playerBounds;
        verticalCheckBounds.position.y += velocity.y; // Add current Y velocity.

        // Determine the range of tile grid coordinates the predicted bounds overlap.
        // Use static_cast to convert float pixel coordinates to integer tile indices.
        // Apply COLLISION_EPSILON to check slightly inside the bounds.
        int leftTileV = static_cast<int>((verticalCheckBounds.position.x + COLLISION_EPSILON) / TILE_SIZE);
        int rightTileV = static_cast<int>((verticalCheckBounds.position.x + verticalCheckBounds.size.x - COLLISION_EPSILON) / TILE_SIZE);
        int topTileV = static_cast<int>((verticalCheckBounds.position.y + COLLISION_EPSILON) / TILE_SIZE);
        int bottomTileV = static_cast<int>((verticalCheckBounds.position.y + verticalCheckBounds.size.y - COLLISION_EPSILON) / TILE_SIZE);

        // Loop through the columns the player might collide with vertically.
        for (int x = leftTileV; x <= rightTileV; ++x) {
            // Check for collision below (landing on a tile). Only check if moving down (velocity.y > 0).
            if (velocity.y > 0 && level.getTile(x, bottomTileV) == Solid) {
                // Collision detected!
                // Reposition the player so their bottom edge rests exactly on top of the solid tile.
                shape.setPosition({shape.getPosition().x, (float)bottomTileV * TILE_SIZE - shape.getSize().y / 2.f});
                // Stop downward movement.
                velocity.y = 0;
                // Set the flag indicating the player is now grounded.
                isOnGround = true;
                // IMPORTANT: Update the main playerBounds variable to reflect the position change,
                // as this corrected position is needed for the subsequent horizontal check.
                playerBounds = shape.getGlobalBounds();
                // Collision resolved for this axis, exit the loop.
                break;
            }
            // Check for collision above (hitting a ceiling). Only check if moving up (velocity.y < 0).
            if (velocity.y < 0 && level.getTile(x, topTileV) == Solid) {
                 // Collision detected!
                 // Reposition the player so their top edge is exactly below the solid tile.
                shape.setPosition({shape.getPosition().x, (float)(topTileV + 1) * TILE_SIZE + shape.getSize().y / 2.f});
                // Stop upward movement.
                velocity.y = 0;
                 // Update playerBounds after the position change.
                playerBounds = shape.getGlobalBounds();
                // Collision resolved, exit the loop.
                break;
            }
        }

        // --- Horizontal Collision Check ---
        // Check collisions along the X-axis *after* vertical collisions are resolved.
        // Uses the potentially updated playerBounds from the vertical check.

        // Predict the horizontal position.
        sf::FloatRect horizontalCheckBounds = playerBounds;
        horizontalCheckBounds.position.x += velocity.x;

        // Determine the tile range for the predicted horizontal bounds.
        int leftTileH = static_cast<int>((horizontalCheckBounds.position.x + COLLISION_EPSILON) / TILE_SIZE);
        int rightTileH = static_cast<int>((horizontalCheckBounds.position.x + horizontalCheckBounds.size.x - COLLISION_EPSILON) / TILE_SIZE);
        // Use the *current* vertical tile range (after vertical adjustments).
        int topTileH = static_cast<int>((playerBounds.position.y + COLLISION_EPSILON) / TILE_SIZE);
        int bottomTileH = static_cast<int>((playerBounds.position.y + playerBounds.size.y - COLLISION_EPSILON) / TILE_SIZE);

        // Loop through the rows the player might collide with horizontally.
        for (int y = topTileH; y <= bottomTileH; ++y) {
             // Check for collision to the right (only if moving right).
            if (velocity.x > 0 && level.getTile(rightTileH, y) == Solid) {
                // Collision detected!
                // Reposition player so their right edge is against the left edge of the tile.
                shape.setPosition({(float)rightTileH * TILE_SIZE - shape.getSize().x / 2.f, shape.getPosition().y});
                // Stop rightward movement.
                velocity.x = 0;
                // Collision resolved, exit the loop.
                break;
            }
            // Check for collision to the left (only if moving left).
            if (velocity.x < 0 && level.getTile(leftTileH, y) == Solid) {
                 // Collision detected!
                 // Reposition player so their left edge is against the right edge of the tile.
                shape.setPosition({(float)(leftTileH + 1) * TILE_SIZE + shape.getSize().x / 2.f, shape.getPosition().y});
                // Stop leftward movement.
                velocity.x = 0;
                // Collision resolved, exit the loop.
                break;
            }
        }
    } // End handleCollision

    // Handles collisions with the outer boundaries of the entire level map.
    void handleLevelBounds(const Level& level) {
        // Get player's current center position and half-size for easier boundary checks.
        sf::Vector2f playerPos = shape.getPosition();
        sf::Vector2f playerHalfSize = shape.getSize() / 2.f;

        // Check left level boundary (position 0)
        if (playerPos.x - playerHalfSize.x < 0.f) {
            // Player's left edge is past the boundary.
            // Reposition player so their left edge is exactly at the boundary.
            shape.setPosition({playerHalfSize.x, playerPos.y});
            // Stop any further leftward movement.
            velocity.x = 0;
        }
        // Check right level boundary (level.sizePixels.x)
        if (playerPos.x + playerHalfSize.x > level.sizePixels.x) {
            // Player's right edge is past the boundary.
            // Reposition player so their right edge is exactly at the boundary.
            shape.setPosition({level.sizePixels.x - playerHalfSize.x, playerPos.y});
            // Stop any further rightward movement.
            velocity.x = 0;
        }
         // Check top level boundary (position 0)
        if (playerPos.y - playerHalfSize.y < 0.f) {
            // Player's top edge is past the boundary.
            // Reposition player so their top edge is exactly at the boundary.
            shape.setPosition({playerPos.x, playerHalfSize.y});
            // Stop any further upward movement.
            velocity.y = 0;
        }
        // Check bottom level boundary (fall out of world)
        if (playerPos.y + playerHalfSize.y > level.sizePixels.y) {
            // Player's bottom edge is past the boundary (they fell off).
            // Example reset behavior: Print message, reset position and velocity.
            std::cout << "Player fell out of bounds!" << std::endl;
            // Reset to the initial starting position (adjust as needed).
            shape.setPosition({TILE_SIZE * 1.5f, TILE_SIZE * (level.size.y - 3.f)});
            velocity = {0.f, 0.f}; // Reset velocity too.
            isOnGround = false; // May not be on ground after reset.
        }
    }


    // Applies the current velocity to the player's shape position.
    // Called after all physics and collision checks for the frame are done.
    void updatePosition() {
        // `move` is a member function of sf::Transformable (base class for shapes/sprites).
        // It adds the given vector (velocity) to the object's current position.
        shape.move(velocity);
    }
}; // End of Player struct

// --- Helper Functions ---

// Creates a simple, hardcoded level map for demonstration.
Level createSimpleLevel() {
    Level level;
    // Set level dimensions in tiles. Made wider to demonstrate scrolling.
    level.size = {40, 15};
    // Calculate and store the total level size in pixels.
    level.sizePixels = {(float)level.size.x * TILE_SIZE, (float)level.size.y * TILE_SIZE};
    // Allocate the 2D vector for the tile data, initializing all to Air.
    level.tiles.resize(level.size.y, std::vector<TileType>(level.size.x, Air));

    // --- Define Solid Tiles ---
    // Floor
    for (int x = 0; x < level.size.x; ++x) {
        level.tiles[level.size.y - 1][x] = Solid;
    }
    // Platforms
    for (int x = 5; x < 10; ++x) level.tiles[10][x] = Solid;
    for (int x = 12; x < 16; ++x) level.tiles[8][x] = Solid;
    level.tiles[6][15] = Solid;
    level.tiles[6][16] = Solid;
    for (int x = 25; x < 30; ++x) level.tiles[10][x] = Solid;
    for (int x = 32; x < 36; ++x) level.tiles[7][x] = Solid;
    level.tiles[12][21] = Solid;
    level.tiles[12][22] = Solid;
    // Walls
    for (int y = 11; y < level.size.y -1; ++y) level.tiles[y][2] = Solid;
    for (int y = 6; y < 11; ++y) level.tiles[y][18] = Solid;
    for (int y = 8; y < level.size.y -1; ++y) level.tiles[y][38] = Solid;

    return level; // Return the fully defined level structure.
}

// Draws the level tiles that are currently visible within the camera's view.
void drawLevel(sf::RenderWindow& window, const Level& level) {
    // Create a rectangle shape used to draw each visible solid tile.
    sf::RectangleShape tileShape({(float)TILE_SIZE, (float)TILE_SIZE});

    // --- View Culling Optimization ---
    // Get the current view (camera) being used by the window for rendering.
    sf::View currentView = window.getView();
    // Calculate the world-coordinate bounding box of the current view.
    sf::FloatRect viewBounds;
    // The view's top-left corner is its center minus half its size.
    viewBounds.position = currentView.getCenter() - currentView.getSize() / 2.f;
    viewBounds.size = currentView.getSize();

    // Determine the range of tile indices (min/max x and y) that could possibly
    // overlap with the view's bounding box. Add 1 to end indices to ensure
    // tiles partially overlapping the right/bottom edges are included.
    // Use std::max/std::min to prevent indices from going outside level bounds.
    int startX = std::max(0, static_cast<int>(viewBounds.position.x / TILE_SIZE));
    int endX = std::min((int)level.size.x, static_cast<int>((viewBounds.position.x + viewBounds.size.x) / TILE_SIZE) + 1);
    int startY = std::max(0, static_cast<int>(viewBounds.position.y / TILE_SIZE));
    int endY = std::min((int)level.size.y, static_cast<int>((viewBounds.position.y + viewBounds.size.y) / TILE_SIZE) + 1);

    // Loop only through the potentially visible range of tiles.
    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            // If the tile at this grid position is solid...
            if (level.tiles[y][x] == Solid) {
                // ...set its color and position...
                tileShape.setFillColor(sf::Color::Blue);
                tileShape.setPosition({(float)x * TILE_SIZE, (float)y * TILE_SIZE});
                // ...and draw it to the window's back buffer.
                window.draw(tileShape);
            }
        }
    }
}


// --- Main Game Function ---
int main() {
    // --- Window Setup ---
    // Create the main game window using the defined constants.
    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Scrolling Platformer");
    // Set the vertical sync limit (usually 60 FPS) for smoother rendering.
    window.setFramerateLimit(60);

    // --- Create Level and Player ---
    Level currentLevel = createSimpleLevel(); // Generate the level data.
    // Create the player, placing them at a defined starting point.
    Player player({TILE_SIZE * 1.5f, TILE_SIZE * (currentLevel.size.y - 3.f)});

    // --- View (Camera) Setup ---
    // Create the main game view (camera). Initialize its center at (0,0) - this will
    // be updated quickly - and set its size to match the window dimensions.
    // This means the view initially shows a portion of the world equal to the window size.
    sf::View gameView({0.f, 0.f}, {(float)WINDOW_WIDTH, (float)WINDOW_HEIGHT});
    // Optional: Center the view on the player's starting position immediately.
    // gameView.setCenter(player.shape.getPosition());


    // --- Game Loop ---
    // The main loop runs continuously, processing one frame of the game per iteration.
    // Order of operations within the loop is important: Events -> Input -> Update -> Draw.
    while (window.isOpen()) { // Loop continues as long as the window shouldn't close.

        // --- 1. Event Handling ---
        // Process window events (close button, keyboard presses/releases, mouse clicks, etc.)
        std::optional<sf::Event> optEvent;
        // Check for events in the queue. Use extra parentheses around assignment for clarity.
        while ((optEvent = window.pollEvent())) {
            // Check if the user clicked the window's close button.
            if (optEvent->is<sf::Event::Closed>()) {
                window.close(); // Signal the window (and game loop) to close.
            }
            // Handle discrete key presses (actions that happen once per press).
            if (optEvent->is<sf::Event::KeyPressed>()) {
                // Safely get the KeyPressed event data.
                if(auto* keyPressed = optEvent->getIf<sf::Event::KeyPressed>()) {
                    // Check the physical key location (scancode).
                    if (keyPressed->scancode == sf::Keyboard::Scan::Space || keyPressed->scancode == sf::Keyboard::Scan::Up) {
                        player.jump(); // Trigger the player's jump action.
                    }
                }
            }
        } // End of event polling loop

        // --- 2. Input Handling (Continuous) ---
        // Check the state of keys for actions that happen while held down (movement).
        player.velocity.x = 0; // Reset horizontal velocity (player stops if no key is pressed).
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
            player.velocity.x = -PLAYER_MOVE_SPEED; // Set velocity for left movement.
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
            player.velocity.x = PLAYER_MOVE_SPEED; // Set velocity for right movement.
        }

        // --- 3. Game Logic / Updates ---
        // Update the state of all game objects based on physics, input, AI, etc.
        player.applyGravity();                // Apply gravity to the player.
        player.handleCollision(currentLevel); // Resolve collisions with solid tiles.
        player.handleLevelBounds(currentLevel);// Resolve collisions with level edges.
        player.updatePosition();              // Apply final velocity to move the player.

        // --- Update View Position ---
        // Center the camera (view) on the player's current position.
        sf::Vector2f viewCenter = player.shape.getPosition();

        // --- Clamp View to Level Boundaries ---
        // Prevent the camera from showing areas outside the defined level map.
        // Calculate the minimum/maximum allowed X/Y coordinates for the view's *center*.
        // The view center cannot go so far left/up that the view's left/top edge goes past 0.
        // The view center cannot go so far right/down that the view's right/bottom edge goes past the level size.
        float minViewX = gameView.getSize().x / 2.f;
        float maxViewX = currentLevel.sizePixels.x - gameView.getSize().x / 2.f;
        float minViewY = gameView.getSize().y / 2.f;
        float maxViewY = currentLevel.sizePixels.y - gameView.getSize().y / 2.f;

        // Handle cases where the level is smaller than the view (no scrolling needed).
        if (currentLevel.sizePixels.x < gameView.getSize().x) {
            minViewX = maxViewX = currentLevel.sizePixels.x / 2.f; // Center horizontally.
        }
         if (currentLevel.sizePixels.y < gameView.getSize().y) {
            minViewY = maxViewY = currentLevel.sizePixels.y / 2.f; // Center vertically.
        }

        // Use std::clamp to restrict the calculated viewCenter within the allowed min/max range.
        viewCenter.x = std::clamp(viewCenter.x, minViewX, maxViewX);
        viewCenter.y = std::clamp(viewCenter.y, minViewY, maxViewY);

        // Apply the potentially clamped center position to the actual game view.
        gameView.setCenter(viewCenter);


        // --- 4. Rendering ---
        // Draw the visual representation of the game state to the window.

        // Clear the previous frame's content with a background color.
        window.clear(sf::Color(100, 150, 255));

        // *** Apply the game view ***
        // This tells SFML to draw subsequent objects relative to the gameView's
        // position and zoom level, effectively applying the camera.
        window.setView(gameView);

        // Draw elements that exist within the game world (affected by the camera).
        drawLevel(window, currentLevel); // Draw the visible parts of the level.
        window.draw(player.shape);       // Draw the player.

        // --- Optional: Draw HUD/UI Elements ---
        // If you had a score display, health bar, etc., that should stay fixed on the
        // screen regardless of camera movement, you would:
        // 1. Reset the view to the window's default view:
        //    window.setView(window.getDefaultView());
        // 2. Draw the HUD elements using screen coordinates:
        //    window.draw(scoreTextHUD); // Assuming scoreTextHUD was defined and positioned earlier.

        // Display the completed frame on the window.
        window.display();
    } // End of main game loop

    return 0; // Indicate successful program termination.
} // End of main function
