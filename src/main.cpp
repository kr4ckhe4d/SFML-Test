// Include the main SFML Graphics header.
// This brings in all the necessary classes for 2D graphics, windowing, and events.
#include <SFML/Graphics.hpp>
// Include the C++ standard library header for std::optional.
// SFML 3 uses std::optional for functions like pollEvent that might or might not return a value.
#include <optional>
// Include the C++ standard library header for std::string operations, specifically std::to_string.
#include <string>
// Include the C++ standard library header for input/output streams, specifically std::cerr for error messages.
#include <iostream>
// Include the C++ standard library header for mathematical functions, specifically std::sqrt for vector normalization.
#include <cmath>
// Include the C++ standard library header for filesystem operations, specifically std::filesystem::path for file paths.
// Required for SFML 3's font loading function.
#include <filesystem>

// The main function - entry point of the program.
int main() {
    // --- Window Setup ---
    // Create the main application window.
    // sf::VideoMode({width, height}) defines the dimensions of the window's client area (in pixels).
    // The second argument is the title string displayed in the window's title bar.
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Simple SFML 3 Game");
    // Set a framerate limit for the window (e.g., 60 frames per second).
    // This helps control the application's speed and reduces CPU usage.
    window.setFramerateLimit(60);

    // --- Font and Text Setup ---
    // Create an sf::Font object to hold font data.
    sf::Font font;
    // Define the path to the font file using std::filesystem::path.
    // IMPORTANT: Ensure this font file (e.g., "arial.ttf") exists in the same
    // directory as the compiled executable, or provide the full, correct path.
    std::filesystem::path fontPath = "arial.ttf";
    // Attempt to load the font data from the specified file path.
    // SFML 3 uses openFromFile for loading resources like fonts.
    if (!font.openFromFile(fontPath)) {
        // If loading fails, print an error message to the standard error stream.
        std::cerr << "Error: Could not load font '" << fontPath.string() << "'. Make sure it's in the correct path." << std::endl;
        // Return a non-zero value to indicate an error occurred.
        return 1;
    }

    // Create an sf::Text object to display text on the screen.
    // Initialize it with the initial string, the loaded font object, and the character size (in pixels).
    // Note: Character size needs to be explicitly cast to unsigned int for the SFML 3 constructor.
    sf::Text scoreText(font,"Score: 0");
    // Set the color of the text. sf::Color provides predefined colors.
    scoreText.setFillColor(sf::Color::White);
    // Set the position of the text's top-left corner on the window.
    // Uses braced initialization {} for the sf::Vector2f position.
    scoreText.setPosition({10.f, 10.f});

    // --- Game Object Setup ---
    // Player (Circle)
    // Create an sf::CircleShape object. The argument is the radius in pixels.
    sf::CircleShape player(30.f);
    // Set the fill color of the circle.
    player.setFillColor(sf::Color::Green);
    // Set the origin of the shape to its center (radius, radius).
    // This makes rotation and positioning around the center point easier.
    player.setOrigin({player.getRadius(), player.getRadius()});
    // Set the initial position of the player's origin on the window.
    player.setPosition({100.f, 100.f});

    // Enemy (Square)
    // Create an sf::RectangleShape object. The argument is the size {width, height}.
    sf::RectangleShape enemy({50.f, 50.f});
    // Set the fill color of the rectangle.
    enemy.setFillColor(sf::Color::Red);
    // Set the origin of the rectangle to its center.
    enemy.setOrigin({enemy.getSize().x / 2.f, enemy.getSize().y / 2.f});
    // Set the initial position of the enemy's origin on the window.
    enemy.setPosition({400.f, 300.f});

    // --- Game Variables ---
    // Define the speed at which the player moves (pixels per frame, adjusted by normalization).
    float moveSpeed = 5.0f;
    // Initialize the player's score to zero.
    int score = 0;

    // --- Game Loop ---
    // The main loop continues as long as the window is open.
    while (window.isOpen()) {
        // --- Event Handling ---
        // Process events that have occurred since the last loop iteration.
        // sf::Event is a union holding data about different event types.
        std::optional<sf::Event> optEvent; // SFML 3 returns an optional event.
        // window.pollEvent() tries to retrieve an event from the queue.
        // If an event is available, it's assigned to optEvent, and the loop continues.
        // If no event is pending, pollEvent returns an empty optional, ending the inner while loop.
        while (optEvent = window.pollEvent()) {
            // Check the type of the event using the is<T>() member function.
            // We access the optional's value using the -> operator.
            if (optEvent->is<sf::Event::Closed>()) {
                // If the user clicked the window's close button, close the window.
                // This will cause window.isOpen() to return false, exiting the main game loop.
                window.close();
            }
            // Other event types (keyboard presses, mouse clicks, etc.) could be handled here
            // using else if (optEvent->is<...>() ) checks.
        }

        // --- Input Handling (Real-time Keyboard Input) ---
        // Check the current state of keys directly (polled every frame).
        // This is suitable for continuous actions like movement.
        sf::Vector2f movement(0.f, 0.f); // Create a 2D vector to store the desired movement direction for this frame.
        // Check if specific arrow keys are currently held down.
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
            movement.x -= 1.f; // Intend to move left.
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
            movement.x += 1.f; // Intend to move right.
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
            movement.y -= 1.f; // Intend to move up (Y is typically down in screen coordinates).
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
            movement.y += 1.f; // Intend to move down.
        }

        // Normalize the movement vector (optional but good practice).
        // This prevents faster movement when moving diagonally.
        // Calculate the length (magnitude) of the movement vector.
        float length = std::sqrt(movement.x * movement.x + movement.y * movement.y);
        // If the length is not zero (i.e., a key was pressed), normalize it.
        if (length != 0) {
            // Divide the movement vector by its length to make it a unit vector (length 1).
            movement /= length;
        }

        // std::cout << "Movement Vector: (" << movement.x << ", " << movement.y << ")" << std::endl;

        // Apply the calculated movement to the player.
        // Multiply the normalized direction vector by the desired speed.
        player.move(movement * moveSpeed);


        // --- Game Logic / Updates ---

        // Collision Detection (using Axis-Aligned Bounding Boxes - AABB).
        // This is a simple rectangle-based collision check.
        // Get the global bounding rectangle of the player shape.
        // This takes into account position, scale, and rotation (though we don't rotate here).
        sf::FloatRect playerBounds = player.getGlobalBounds();
        // Get the global bounding rectangle of the enemy shape.
        sf::FloatRect enemyBounds = enemy.getGlobalBounds();

        // Check if the player's bounding box intersects with the enemy's bounding box.
        // In SFML 3, findIntersection returns an std::optional<sf::FloatRect>.
        // .has_value() returns true if the optional contains a value (meaning they intersect).
        if (playerBounds.findIntersection(enemyBounds).has_value()) {
            // --- Collision Response ---
            // Increment the score.
            score++;
            // Update the score text string displayed on the screen.
            // std::to_string converts the integer score to a std::string.
            scoreText.setString("Score: " + std::to_string(score));

            // Respawn the enemy at a new position.
            // This uses simple modulo arithmetic for a pseudo-random effect.
            // For better randomness, use the C++ <random> library.
            float newX = static_cast<float>((score * 150) % 700 + 50); // Calculate a new X position.
            float newY = static_cast<float>((score * 250) % 500 + 50); // Calculate a new Y position.
            // Set the enemy's new position.
            enemy.setPosition({newX, newY});
        }

        // Keep player within screen bounds (optional boundary check).
        // Get the current size of the window.
        sf::Vector2u windowSize = window.getSize();
        // Get the current position of the player's origin (center).
        sf::Vector2f playerPos = player.getPosition();
        // Get the player's radius.
        float playerRadius = player.getRadius();

        // Check and correct position if player hits screen edges.
        // Uses the player's center position and radius for calculations.
        if (playerPos.x - playerRadius < 0.f) player.setPosition({playerRadius, playerPos.y}); // Left edge
        if (playerPos.x + playerRadius > windowSize.x) player.setPosition({(float)windowSize.x - playerRadius, playerPos.y}); // Right edge
        if (playerPos.y - playerRadius < 0.f) player.setPosition({playerPos.x, playerRadius}); // Top edge
        if (playerPos.y + playerRadius > windowSize.y) player.setPosition({playerPos.x, (float)windowSize.y - playerRadius}); // Bottom edge


        // --- Rendering ---
        // This section draws everything to the window for the current frame.
        // It should happen after all game logic and updates.

        // Clear the entire window with a specific color (e.g., black).
        // This is usually done first in each frame to erase the previous frame's contents.
        window.clear(sf::Color::Black);

        // Draw the game objects onto the window's back buffer (not yet visible on screen).
        // The order of drawing matters; objects drawn later appear on top.
        window.draw(player);    // Draw the player circle.
        window.draw(enemy);     // Draw the enemy rectangle.
        window.draw(scoreText); // Draw the score text.

        // Display the contents of the back buffer on the screen.
        // This swaps the buffers and makes everything drawn since the last clear visible.
        window.display();
    } // End of the main game loop (while window.isOpen())

    // Return 0 from main to indicate successful program execution.
    return 0;
} // End of the main function
