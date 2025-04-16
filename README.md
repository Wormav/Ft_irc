# Ft_irc

An IRC (Internet Relay Chat) server implementation in C++.

## Project Architecture

The project is structured into several components, each responsible for a specific aspect of the IRC server:

- **Server**: Manages client connections, command processing, and overall server lifecycle.
- **User**: Represents a connected client, including their authentication state and identity.
- **Channel**: Represents an IRC channel where users can join and communicate.
- **Command**: Handles parsing and execution of IRC protocol commands.
- **Bot** (Bonus): Implements a bot that interacts with users in the IRC server.

### Component Relationships

```
Server
 ├── manages multiple Users (connected clients)
 ├── maintains multiple Channels
 └── uses Command handler for processing client messages
```

## Code Functionality

### Server Workflow

1. **Initialization**: The server is initialized with a port and password.
2. **Socket Setup**: A socket is created and configured to listen for incoming connections.
3. **Main Loop**: The server uses `epoll` to handle multiple connections asynchronously.
4. **Client Handling**:
   - Accepts new client connections.
   - Reads data from clients and processes commands.
   - Sends responses back to clients.
5. **Shutdown**: Cleans up resources when the server is stopped.

### Command Processing Flow

1. A client sends a command to the server.
2. The server reads the command and passes it to the `Command` handler.
3. The `Command` handler parses the command and executes the corresponding method.
4. Responses are sent back to the client or broadcasted to other clients as needed.

## Class Documentation

### Server Class

Manages the core functionality of the IRC server.

| Method | Description |
|--------|-------------|
| `Server(int port, const std::string& password)` | Initializes the server with a port and password. |
| `~Server()` | Cleans up resources. |
| `run()` | Starts the server's main loop. |
| `disconnectClient(int client_fd)` | Disconnects a client and cleans up their resources. |
| `cleanupResources()` | Frees all resources used by the server. |
| `setupSocket()` | Configures the server socket for incoming connections. |
| `handleNewConnection()` | Accepts a new client connection. |
| `handleClientData(int client_fd)` | Processes data received from a client. |
| `processCommand(int client_fd, const std::string& line)` | Passes a command to the `Command` handler. |

---

### User Class

Represents a connected client.

| Method | Description |
|--------|-------------|
| `User()` | Initializes a new user. |
| `~User()` | Destructor. |
| `getNickname() const` | Returns the user's nickname. |
| `getUsername() const` | Returns the user's username. |
| `getRealname() const` | Returns the user's real name. |
| `isAuthenticated() const` | Checks if the user is authenticated. |
| `isPasswordVerified() const` | Checks if the user has verified the server password. |
| `setNickname(const std::string& nick)` | Sets the user's nickname. |
| `setUsername(const std::string& user)` | Sets the user's username. |
| `setRealname(const std::string& real)` | Sets the user's real name. |
| `setAuthenticated(bool auth)` | Updates the user's authentication status. |
| `setPasswordVerified(bool verified)` | Updates the password verification status. |
| `getFullIdentity() const` | Returns the user's full identity string. |

---

### Channel Class

Represents an IRC channel.

| Method | Description |
|--------|-------------|
| `Channel()` | Default constructor. |
| `Channel(const std::string& channelName)` | Initializes a channel with a name. |
| `~Channel()` | Destructor. |
| `getName() const` | Returns the channel name. |
| `getTopic() const` | Returns the channel topic. |
| `getMembers() const` | Returns the set of members in the channel. |
| `getOperators() const` | Returns the set of operators in the channel. |
| `setName(const std::string& channelName)` | Sets the channel name. |
| `setTopic(const std::string& channelTopic)` | Sets the channel topic. |
| `addMember(int client_fd)` | Adds a member to the channel. |
| `removeMember(int client_fd)` | Removes a member from the channel. |
| `hasMember(int client_fd) const` | Checks if a user is a member of the channel. |
| `isEmpty() const` | Checks if the channel has no members. |
| `addOperator(int client_fd)` | Adds an operator to the channel. |
| `removeOperator(int client_fd)` | Removes an operator from the channel. |
| `isOperator(int client_fd) const` | Checks if a user is an operator. |
| `addInvite(int client_fd)` | Invites a user to the channel. |
| `removeInvite(int client_fd)` | Removes an invite for a user. |
| `isInvited(int client_fd) const` | Checks if a user is invited. |
| `isInviteOnly() const` | Checks if the channel is invite-only. |
| `isTopicRestricted() const` | Checks if the topic is restricted to operators. |
| `hasKeySet() const` | Checks if the channel has a password. |
| `hasUserLimitSet() const` | Checks if the channel has a user limit. |
| `getKey() const` | Returns the channel password. |
| `getUserLimit() const` | Returns the user limit. |
| `setInviteOnly(bool value)` | Sets the invite-only mode. |
| `setTopicRestricted(bool value)` | Sets the topic restriction mode. |
| `setKey(const std::string& newKey)` | Sets the channel password. |
| `removeKey()` | Removes the channel password. |
| `setUserLimit(size_t limit)` | Sets the user limit. |
| `removeUserLimit()` | Removes the user limit. |
| `getModeString() const` | Returns the channel's mode string. |
| `broadcastMessage(const std::string& message, int excludeClient)` | Sends a message to all members, excluding one if specified. |

---

### Command Class

Handles the parsing and execution of IRC commands.

| Method | Description |
|--------|-------------|
| `Command(Server* server, std::map<int, User>& users, std::map<std::string, Channel>& channels, const std::string& password)` | Initializes the command handler. |
| `~Command()` | Destructor. |
| `process(int client_fd, const std::string& line)` | Processes a command from a client. |
| `sendWelcomeMessages(int client_fd, const User& user)` | Sends welcome messages to a newly authenticated user. |
| `handlePass(int client_fd, std::istringstream& iss)` | Handles the `PASS` command. |
| `handleNick(int client_fd, std::istringstream& iss)` | Handles the `NICK` command. |
| `handleUser(int client_fd, const std::string& line)` | Handles the `USER` command. |
| `handleJoin(int client_fd, std::istringstream& iss)` | Handles the `JOIN` command. |
| `handlePrivmsg(int client_fd, std::istringstream& iss)` | Handles the `PRIVMSG` command. |
| `handlePart(int client_fd, const std::string& line)` | Handles the `PART` command. |
| `handleQuit(int client_fd, const std::string& line)` | Handles the `QUIT` command. |
| `handleKick(int client_fd, const std::string& line)` | Handles the `KICK` command. |
| `handleInvite(int client_fd, const std::string& line)` | Handles the `INVITE` command. |
| `handleTopic(int client_fd, const std::string& line)` | Handles the `TOPIC` command. |
| `handleMode(int client_fd, const std::string& line)` | Handles the `MODE` command. |

---

### Bot Class (Bonus)

Implements a bot for automated interactions.

| Method | Description |
|--------|-------------|
| `Bot(const std::string& nickname, const std::string& username, const std::string& realname, const std::string& channel)` | Initializes the bot with its identity and channel. |
| `~Bot()` | Destructor. |
| `initResponses()` | Initializes the bot's predefined responses. |
| `getRandomResponse() const` | Returns a random response from the bot. |
| `getNickname() const` | Returns the bot's nickname. |
| `getUsername() const` | Returns the bot's username. |
| `getRealname() const` | Returns the bot's real name. |
| `getChannel() const` | Returns the bot's channel. |

## Contributors

- [EricBrvs](https://github.com/EricBrvs)
- [aauberti](https://github.com/aauberti)
- [Mastau](https://github.com/Mastau)
- [Wormav](https://github.com/Wormav)
