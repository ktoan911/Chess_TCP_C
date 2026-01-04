# Chess TCP - Chi Tiết Kỹ Thuật & Giao Thức

---

## 📋 Tổng Quan

**Chess TCP** - Hệ thống chơi cờ vua trực tuyến với kiến trúc Client-Server TCP/IP

**Tech Stack:** C++17 | TCP/IP | Multi-threading | JSON | Chess Library

**Features:** Đăng ký/Đăng nhập | Matchmaking (rank ≤10) | Thách đấu | Lịch sử | ELO/Rank

---

## 🏗️ Kiến Trúc Hệ Thống

```mermaid
graph TB
    subgraph Clients["Multiple Clients"]
        C1[Client 1]
        C2[Client 2]
        CN[Client N]
    end
    
    subgraph Server["Server Port 8088"]
        NS[NetworkServer<br/>Singleton]
        MH[MessageHandler<br/>Router]
        GM[GameManager<br/>Matchmaking]
        DS[DataStorage<br/>JSON]
    end
    
    C1 <-->|TCP| NS
    C2 <-->|TCP| NS
    CN <-->|TCP| NS
    
    NS --> MH
    MH --> GM
    GM --> DS
    
    GM -.->|Background Thread| MM[Matchmaking Loop]
    
    style NS fill:#ff6b6b
    style GM fill:#4ecdc4
    style DS fill:#ffe66d
    style MM fill:#95e1d3
```

---

## 🖥️ Chi Tiết Server

### Server Architecture Flow

Server sử dụng mô hình **Multi-threaded** với mỗi Client được xử lý trên một luồng riêng biệt, kết hợp với các Singleton Manager để quản lý trạng thái chung.

```mermaid
graph TD
    subgraph "Connection Layer (Main Thread)"
        Listen[Listen Port 8088] -->|Accept| Handshake[Handshake]
        Handshake -->|Spawn| Thread[Client Thread]
    end

    subgraph "Request Processing (Per Client Thread)"
        Thread -->|Recv| Parser[Packet Parser]
        Parser -->|Identify| Router{Message Type}
        
        Router -->|Auth/Info| NS_Logic[Network Logic]
        Router -->|Move/Play| GM_Logic[Game Logic]
        
        NS_Logic -->|Write| DB[DataStorage]
        GM_Logic -->|Read/Write| DB
        
        NS_Logic -->|Send| Sender[Response Sender]
        GM_Logic -->|Notify| Sender
    end

    subgraph "Core Systems (Singleton)"
        NS[NetworkServer]
        GM[GameManager]
    end
    
    subgraph "Background Services"
        MM[Matchmaking Loop]
    end

    NS_Logic -.-> NS
    GM_Logic -.-> GM
    MM -.->|Scan Queue| GM
    
    Sender -->|TCP| Client
    
    style Listen fill:#ff9f43
    style Thread fill:#4ecdc4
    style GM_Logic fill:#ff6b6b
    style DB fill:#ffe66d
```

---

## 💻 Chi Tiết Client

### Client Architecture (Event-Driven)

Client sử dụng mô hình **Single-threaded Event Loop** với `poll()` để xử lý đồng thời user input và server messages mà không cần đa luồng phức tạp.

```mermaid
graph TD
    subgraph "Main Event Loop (client_main.cpp)"
        Poll[poll() Wait] -->|Input Event| IP[InputProcessor]
        Poll -->|Network Event| MH[MessageHandler]
    end
    
    subgraph "Core Components"
        IP -->|Update State| SM[State Machine]
        MH -->|Update State| SM
        
        IP -->|Send Packet| NC[NetworkClient]
        MH -->|Read Packet| NC
        
        IP -->|Render| UI[UI Display]
        MH -->|Render| UI
    end
    
    subgraph "Data Layer"
        SD[SessionData]
        SC[StateContext]
    end
    
    IP -.-> SD & SC
    MH -.-> SD & SC
    
    NC <-->|TCP/IP| Server
    User[User Keyboard] -->|stdin| Poll
    
    style Poll fill:#ff9f43
    style IP fill:#4ecdc4
    style MH fill:#ff6b6b
    style UI fill:#ffe66d
```

### Client State Machine

Client hoạt động dựa trên một State Machine chặt chẽ để quản lý luồng màn hình và input hợp lệ.

```mermaid
stateDiagram-v2
    [*] --> INITIAL_MENU
    
    INITIAL_MENU --> WAITING_AUTH: Login/Register
    WAITING_AUTH --> GAME_MENU: Success
    WAITING_AUTH --> INITIAL_MENU: Failure
    
    GAME_MENU --> WAITING_MATCH: Auto Match
    GAME_MENU --> PLAYER_LIST: View Players
    
    WAITING_MATCH --> MATCH_DECISION: Found
    MATCH_DECISION --> IN_GAME: Accept
    
    PLAYER_LIST --> CHALLENGE_WAIT: Send Challenge
    CHALLENGE_WAIT --> IN_GAME: Accepted
    
    GAME_MENU --> CHALLENGE_RECV: Receive Challenge
    CHALLENGE_RECV --> IN_GAME: Accept
    
    state IN_GAME {
        [*] --> MY_TURN
        MY_TURN --> OPPONENT_TURN: Move
        OPPONENT_TURN --> MY_TURN: Receive Move
    }
    
    IN_GAME --> GAME_MENU: Game End
```

### Các Module Client Chính

| Module | Vai trò | Chi tiết |
|--------|---------|----------|
| **client_main** | Entry Point | Thiết lập `poll()`, quản lý vòng lặp chính, xử lý signal SIGINT. |
| **NetworkClient** | Singleton | Quản lý socket, buffer partial packets, gửi/nhận non-blocking. |
| **InputProcessor** | Input Logic | Xử lý phím bấm, validate input theo state hiện tại, gửi request. |
| **MessageHandler** | Response Logic | Xử lý packet từ server, cập nhật state, hiển thị thông báo/bàn cờ. |
| **UI / BoardDisplay** | Rendering | Vẽ giao diện ANSI color, render bàn cờ từ FEN string, hỗ trợ flip board. |
| **SessionData** | Data Store | Lưu username, ELO, trạng thái game (FEN, turn, color). |

### Kỹ Thuật UI & Input

- **Non-blocking Input**: Sử dụng `termios` để tắt canonical mode & echo, cho phép đọc từng phím bấm ngay lập tức.
- **ANSI Colors**: Sử dụng mã màu ANSI để hiển thị bàn cờ đẹp mắt trên terminal.
- **Board Rendering**: Parse chuỗi FEN (Forsyth–Edwards Notation) để vẽ bàn cờ, tự động xoay bàn cờ nếu người chơi cầm quân Đen.

---

## 📡 Giao Thức Chi Tiết

### Cấu Trúc Packet

```
┌──────────┬────────────┬─────────────────┐
│   Type   │   Length   │     Payload     │
│  1 byte  │  2 bytes   │   N bytes       │
│  uint8   │ Big-Endian │   Variable      │
└──────────┴────────────┴─────────────────┘
```

### Message Types Đầy Đủ

| Hex | Type | Direction | Payload | Mô tả |
|-----|------|-----------|---------|-------|
| **Test & Response** |
| `0x00` | TEST | C→S | - | Kiểm tra kết nối |
| `0x01` | RESPONSE | S→C | `string message` | Phản hồi chung |
| **Authentication** |
| `0x10` | REGISTER | C→S | `string username` | Đăng ký tài khoản |
| `0x11` | REGISTER_SUCCESS | S→C | `string username, u16 elo` | Đăng ký thành công |
| `0x12` | REGISTER_FAILURE | S→C | `string error` | Lỗi đăng ký |
| `0x20` | LOGIN | C→S | `string username` | Đăng nhập |
| `0x21` | LOGIN_SUCCESS | S→C | `string username, u16 elo, u16 rank` | Đăng nhập OK |
| `0x22` | LOGIN_FAILURE | S→C | `string error` | Lỗi đăng nhập |
| **Player Management** |
| `0x30` | REQUEST_PLAYER_LIST | C→S | - | Xin danh sách online |
| `0x31` | PLAYER_LIST | S→C | `u16 count, [Player...]` | DS người chơi |
| **Game Core** |
| `0x40` | GAME_START | S→C | `string game_id, FEN, white, black, u16 elo_w, elo_b` | Bắt đầu ván |
| `0x41` | MOVE | C→S | `string game_id, uci_move` | Gửi nước đi |
| `0x42` | INVALID_MOVE | S→C | `string game_id, error` | Nước đi không hợp lệ |
| `0x43` | GAME_STATUS_UPDATE | S→C | `string game_id, FEN, turn, bool in_check` | Cập nhật bàn cờ |
| `0x44` | GAME_END | S→C | `string game_id, result, reason, i16 elo_change` | Kết thúc |
| `0x45` | SURRENDER | C→S | `string game_id` | Đầu hàng |
| `0x46` | GAME_LOG | S→C | `string game_id, [Move...]` | Lịch sử nước đi |
| **Challenge** |
| `0x50` | CHALLENGE_REQUEST | C→S | `string opponent` | Thách đấu |
| `0x51` | CHALLENGE_NOTIFICATION | S→C | `string challenger, u16 elo` | Nhận lời thách |
| `0x52` | CHALLENGE_RESPONSE | C→S | `string challenger, bool accept` | Chấp nhận/Từ chối |
| `0x53` | CHALLENGE_ACCEPTED | S→C | - | Đối thủ đồng ý |
| `0x54` | CHALLENGE_DECLINED | S→C | `string reason` | Đối thủ từ chối |
| `0x5B` | CHALLENGE_ERROR | S→C | `string error` | Lỗi thách đấu |
| **Auto Matchmaking** |
| `0x55` | AUTO_MATCH_REQUEST | C→S | - | Tìm trận tự động |
| `0x56` | AUTO_MATCH_FOUND | S→C | `string game_id, opponent, u16 elo` | Tìm thấy đối thủ |
| `0x57` | AUTO_MATCH_ACCEPTED | C→S | `string game_id` | Chấp nhận trận |
| `0x58` | AUTO_MATCH_DECLINED | C→S | `string game_id` | Từ chối trận |
| `0x59` | AUTO_MATCH_DECLINED_NOTIFICATION | S→C | `string game_id` | Thông báo hủy |

---

## 🔄 Sequence Diagrams Chi Tiết

### 1. Luồng Đăng Ký

```mermaid
sequenceDiagram
    actor U as User
    participant C as Client
    participant NS as NetworkServer
    participant MH as MessageHandler
    participant DS as DataStorage
    
    U->>C: Nhập username
    C->>NS: REGISTER(0x10)<br/>username
    NS->>MH: Route message
    MH->>DS: checkUserExists(username)
    
    alt Username đã tồn tại
        DS-->>MH: User exists
        MH->>NS: REGISTER_FAILURE(0x12)
        NS->>C: error="Username đã tồn tại"
        C->>U: Hiển thị lỗi
    else Username OK
        DS-->>MH: OK
        MH->>DS: createUser(username)
        DS->>DS: Khởi tạo ELO=1200
        DS-->>MH: User created
        MH->>NS: REGISTER_SUCCESS(0x11)
        NS->>C: username, elo=1200
        C->>U: "Đăng ký thành công"
    end
```

### 2. Luồng Đăng Nhập

```mermaid
sequenceDiagram
    participant C as Client
    participant NS as NetworkServer
    participant MH as MessageHandler
    participant DS as DataStorage
    
    C->>NS: LOGIN(0x20)<br/>username
    NS->>MH: Route
    
    MH->>NS: isUserLoggedIn(username)
    alt Đã đăng nhập
        NS-->>MH: true
        MH->>C: LOGIN_FAILURE(0x22)<br/>"User đã online"
    else Chưa đăng nhập
        MH->>DS: validateUser(username)
        alt User không tồn tại
            DS-->>MH: not found
            MH->>C: LOGIN_FAILURE(0x22)<br/>"Username không tồn tại"
        else User OK
            DS-->>MH: elo, rank
            NS->>NS: setUsername(fd, username)
            MH->>C: LOGIN_SUCCESS(0x21)<br/>username, elo, rank
            Note over C: Chuyển sang Game Menu
        end
    end
```

### 3. Matchmaking Flow (Chi Tiết)

```mermaid
sequenceDiagram
    participant C1 as Client 1<br/>(rank=5)
    participant C2 as Client 2<br/>(rank=8)
    participant NS as Server
    participant GM as GameManager
    participant MM as Matchmaking Thread
    participant DS as DataStorage
    
    rect rgb(200, 220, 240)
        Note over C1,C2: PHASE 1: Join Queue
        C1->>NS: AUTO_MATCH_REQUEST(0x55)
        NS->>GM: addPlayerToQueue(fd1)
        GM->>GM: matchmaking_queue.push(fd1)
        
        C2->>NS: AUTO_MATCH_REQUEST(0x55)
        NS->>GM: addPlayerToQueue(fd2)
        GM->>GM: matchmaking_queue.push(fd2)
    end
    
    rect rgb(220, 240, 220)
        Note over MM: PHASE 2: Background Matching
        loop Every 1 second
            MM->>MM: Lock matchmaking_mutex
            MM->>MM: queue.size() >= 2?
            MM->>DS: getUserRank(fd1) → rank1=5
            MM->>DS: getUserRank(fd2) → rank2=8
            MM->>MM: |5-8| = 3 ≤ 10 ✓
            MM->>MM: createPendingGame(fd1, fd2)
            MM->>MM: gameId = generateUUID()
        end
    end
    
    rect rgb(255, 235, 235)
        Note over C1,C2: PHASE 3: Match Found
        MM->>C1: AUTO_MATCH_FOUND(0x56)<br/>gameId, opponent="C2", elo
        MM->>C2: AUTO_MATCH_FOUND(0x56)<br/>gameId, opponent="C1", elo
        
        C1->>NS: AUTO_MATCH_ACCEPTED(0x57)<br/>gameId
        NS->>GM: handleAutoMatchAccepted(fd1, gameId)
        GM->>GM: pending[gameId].player1_accepted = true
        
        C2->>NS: AUTO_MATCH_ACCEPTED(0x57)<br/>gameId
        NS->>GM: handleAutoMatchAccepted(fd2, gameId)
        GM->>GM: pending[gameId].player2_accepted = true
    end
    
    rect rgb(235, 255, 235)
        Note over C1,C2: PHASE 4: Start Game
        GM->>GM: Both accepted?<br/>→ createGame()
        GM->>GM: Random white/black
        GM->>DS: saveMatch(gameId)
        GM->>C1: GAME_START(0x40)<br/>gameId, FEN, white, black, ELOs
        GM->>C2: GAME_START(0x40)<br/>gameId, FEN, white, black, ELOs
        Note over C1,C2: Hiển thị bàn cờ
    end
```

### 4. Game Play Loop (Chi Tiết Từng Bước)

```mermaid
sequenceDiagram
    participant P1 as Player 1 (White)
    participant NS as NetworkServer
    participant GM as GameManager
    participant GS as GameStatus
    participant CE as Chess Engine
    participant P2 as Player 2 (Black)
    
    Note over P1,P2: Trạng thái: White's turn
    
    rect rgb(240, 248, 255)
        Note over P1: Người chơi nhập nước đi
        P1->>NS: MOVE(0x41)<br/>gameId, "e2e4"
        NS->>GM: handleMove(fd1, gameId, "e2e4")
        
        GM->>GS: getGame(gameId)
        GS-->>GM: game object
        
        GM->>GM: Validate turn<br/>(fd1 == white_fd?)
    end
    
    alt Không phải lượt
        GM->>P1: INVALID_MOVE(0x42)<br/>"Chưa đến lượt bạn"
    else Đúng lượt
        rect rgb(240, 255, 240)
            Note over GM,CE: Validate & Execute
            GM->>CE: makeMove("e2e4")
            CE->>CE: isLegalMove()?
            
            alt Nước đi không hợp lệ
                CE-->>GM: false
                GM->>P1: INVALID_MOVE(0x42)<br/>"Nước đi không hợp lệ"
            else Nước đi hợp lệ
                CE->>CE: updateBoard()
                CE-->>GM: new FEN, turn, check status
                
                GM->>GS: Update game state
                GS->>GS: addMove("e2e4", FEN)
                GS->>GS: switchTurn()
                
                GM->>P1: GAME_STATUS_UPDATE(0x43)<br/>FEN, turn="black", in_check=false
                GM->>P2: GAME_STATUS_UPDATE(0x43)<br/>FEN, turn="black", in_check=false
                
                Note over P1,P2: Cả 2 cập nhật bàn cờ
            end
        end
        
        rect rgb(255, 245, 235)
            Note over GM,CE: Check Game Over
            GM->>CE: isCheckmate()?
            GM->>CE: isStalemate()?
            GM->>CE: isDraw()?
            
            alt Checkmate
                GM->>GM: endGame(gameId, "checkmate")
                GM->>GM: calculateELO(winner, loser)
                GM->>DS: updateUserELO(white, +3)
                GM->>DS: updateUserELO(black, -3)
                GM->>DS: saveMatch(gameId, result)
                
                GM->>P1: GAME_END(0x44)<br/>result="WHITE", reason="checkmate", elo_change=+3
                GM->>P2: GAME_END(0x44)<br/>result="BLACK", reason="checkmate", elo_change=-3
                
                Note over P1,P2: Hiển thị kết quả
            else Game continues
                Note over P2: Đến lượt Black
            end
        end
    end
```

### 5. Challenge Flow

```mermaid
sequenceDiagram
    participant C1 as Client 1
    participant S as Server
    participant GM as GameManager
    participant C2 as Client 2
    
    C1->>S: CHALLENGE_REQUEST(0x50)<br/>"player2"
    S->>GM: Handle challenge
    
    GM->>GM: Validate conditions
    alt Target not online
        GM->>C1: CHALLENGE_ERROR(0x5B)<br/>"Player không online"
    else Target in game
        GM->>C1: CHALLENGE_ERROR(0x5B)<br/>"Player đang chơi"
    else OK
        GM->>C2: CHALLENGE_NOTIFICATION(0x51)<br/>challenger="player1", elo
        
        C2->>C2: User decides...
        
        alt Accept
            C2->>S: CHALLENGE_RESPONSE(0x52)<br/>challenger, accept=true
            S->>GM: Create game
            GM->>C1: CHALLENGE_ACCEPTED(0x53)
            GM->>C1: GAME_START(0x40)<br/>...
            GM->>C2: GAME_START(0x40)<br/>...
        else Decline
            C2->>S: CHALLENGE_RESPONSE(0x52)<br/>challenger, accept=false
            S->>C1: CHALLENGE_DECLINED(0x54)<br/>"Player từ chối"
        end
    end
```

---

## 🔀 Flowcharts Thuật Toán

### Matchmaking Algorithm

```mermaid
flowchart TD
    Start([Matchmaking Thread Start]) --> Wait{Queue size >= 2?}
    Wait -->|No| Sleep[Sleep 1s]
    Sleep --> Wait
    
    Wait -->|Yes| Lock[Lock matchmaking_mutex]
    Lock --> Pop1[Pop player1 từ queue]
    Pop1 --> GetRank1[rank1 = getUserRank player1]
    
    GetRank1 --> TempQ[Create temp_queue]
    TempQ --> Loop{Còn players<br/>trong queue?}
    
    Loop -->|No| Restore[Restore temp_queue<br/>vào matchmaking_queue]
    Restore --> Unlock1[Unlock mutex]
    Unlock1 --> Sleep
    
    Loop -->|Yes| PopCandidate[Pop candidate]
    PopCandidate --> GetRank2[rank2 = getUserRank candidate]
    GetRank2 --> Compare{|rank1 - rank2|<br/><= 10 ?}
    
    Compare -->|No| AddTemp[Add candidate<br/>to temp_queue]
    AddTemp --> Loop
    
    Compare -->|Yes| Match[✓ Match Found!]
    Match --> GenID[gameId = generateUUID]
    GenID --> CreatePending[Create PendingGame<br/>player1, candidate, gameId]
    CreatePending --> SendNotif[Send AUTO_MATCH_FOUND<br/>to both players]
    SendNotif --> StartTimer[Start 15s timeout timer]
    StartTimer --> Unlock2[Unlock mutex]
    Unlock2 --> Wait
    
    style Start fill:#90EE90
    style Match fill:#FFD700
    style SendNotif fill:#87CEEB
```

### Move Validation Flow

```mermaid
flowchart TD
    Start([Receive MOVE message]) --> Extract[Extract gameId, uci_move]
    Extract --> GetGame{Game exists?}
    
    GetGame -->|No| ErrGame[Send INVALID_MOVE<br/>'Game not found']
    ErrGame --> End([End])
    
    GetGame -->|Yes| GetFD[Determine player FD]
    GetFD --> CheckTurn{Is player's turn?}
    
    CheckTurn -->|No| ErrTurn[Send INVALID_MOVE<br/>'Not your turn']
    ErrTurn --> End
    
    CheckTurn -->|Yes| ValidateUCI{UCI format valid?}
    ValidateUCI -->|No| ErrFormat[Send INVALID_MOVE<br/>'Invalid UCI format']
    ErrFormat --> End
    
    ValidateUCI -->|Yes| ChessEngine[Chess Engine:<br/>makeMove uci_move]
    ChessEngine --> Legal{Move legal?}
    
    Legal -->|No| ErrIllegal[Send INVALID_MOVE<br/>'Illegal move']
    ErrIllegal --> End
    
    Legal -->|Yes| UpdateBoard[Update board state<br/>Get new FEN]
    UpdateBoard --> SaveMove[Save move to history]
    SaveMove --> Notify[Send GAME_STATUS_UPDATE<br/>to both players]
    
    Notify --> CheckMate{Checkmate?}
    CheckMate -->|Yes| EndCheckmate[endGame 'checkmate']
    EndCheckmate --> UpdateELO[Update ELO ±3]
    UpdateELO --> SendEnd[Send GAME_END<br/>to both]
    SendEnd --> End
    
    CheckMate -->|No| CheckStale{Stalemate/Draw?}
    CheckStale -->|Yes| EndDraw[endGame 'stalemate']
    EndDraw --> SendEnd
    
    CheckStale -->|No| SwitchTurn[Switch turn]
    SwitchTurn --> End
    
    style Start fill:#90EE90
    style UpdateBoard fill:#FFD700
    style EndCheckmate fill:#FF6B6B
    style End fill:#DDD
```

---

## 🧵 Thread Architecture (Server)

```mermaid
graph TB
    subgraph Server["Server Process"]
        MT[Main Thread<br/>Accept Loop]
        
        subgraph ClientPool["Client Thread Pool"]
            CT1[Client Thread 1]
            CT2[Client Thread 2]
            CTN[Client Thread N]
        end
        
        MMT[Matchmaking Thread<br/>Background Loop]
        
        subgraph Shared["Shared Resources<br/>(Mutex Protected)"]
            GM[GameManager<br/>games_mutex<br/>matchmaking_mutex]
            NS[NetworkServer<br/>clients_mutex]
            DS[DataStorage<br/>No mutex - single thread]
        end
    end
    
    MT -->|spawn| CT1
    MT -->|spawn| CT2
    MT -->|spawn| CTN
    
    MT -->|start once| MMT
    
    CT1 -.->|lock| NS
    CT2 -.->|lock| NS
    CTN -.->|lock| NS
    
    CT1 -.->|lock| GM
    CT2 -.->|lock| GM
    CTN -.->|lock| GM
    
    MMT -.->|lock| GM
    
    GM --> DS
    
    style MT fill:#ff6b6b
    style MMT fill:#4ecdc4
    style GM fill:#ffe66d
    style NS fill:#95e1d3
    style DS fill:#a8e6cf
```

### Mutex cheat sheet

| Resource | Mutex | Scope | Threads |
|----------|-------|-------|---------|
| `games` map | `games_mutex` | GameManager | Client threads, Matchmaking thread |
| `pending_games` map | `games_mutex` | GameManager | Client threads, Matchmaking thread |
| `matchmaking_queue` | `matchmaking_mutex` | GameManager | Client threads, Matchmaking thread |
| `clients` map | `clients_mutex` | NetworkServer | Main thread, Client threads |

---

## 💾 Data Persistence

### users.json
```json
{
  "username": "player1",
  "elo": 1206,
  "match_history": ["uuid-game-1", "uuid-game-2"]
}
```

### matches.json
```json
{
  "game_id": "550e8400-e29b-41d4-a716-446655440000",
  "white_username": "player1",
  "black_username": "player2",
  "moves": [
    {"uci": "e2e4", "fen": "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"},
    {"uci": "e7e5", "fen": "..."}
  ],
  "result": "WHITE",
  "reason": "checkmate",
  "timestamp": 1234567890
}
```

---

## 🎯 Rank System

| ELO Range | Rank |
|-----------|------|
| 0-99 | 0 |
| 100-199 | 1 |
| ... | ... |
| 1200-1299 | 12 |

**ELO Updates:**
- Win: +3
- Loss: -3 (minimum 0)
- Draw: 0
- Disconnect/Surrender: Loss

**Matchmaking:** `|rank1 - rank2| ≤ 10`

---

## 🚀 Build & Run

```bash
make all          # Build server + client
make run_server   # Terminal 1
make run_client   # Terminal 2, 3, ...
```

---

## ⚙️ Possible Extensions

- [ ] Time control (đồng hồ)
- [ ] Chat
- [ ] Spectator mode
- [ ] Tournaments
- [ ] Password authentication
- [ ] PostgreSQL/MongoDB

---

## 📊 Stats

- **Lines:** ~6000+
- **Messages:** 23 types
- **Threads:** 1 main + N clients + 1 matchmaking
- **Patterns:** Singleton, Message Queue, Thread Pool, State Machine

---

## 🎓 Kết Luận

**Highlights:**
✅ Protocol rõ ràng với 23 message types  
✅ Matchmaking algorithm rank-based  
✅ Client Event-Driven & State Machine  
✅ Thread-safe với mutex protection  
✅ Full game lifecycle management  

**Technical Skills:**
- Multi-threaded server architecture
- TCP/IP socket programming
- Protocol design & serialization
- Game state management
- Synchronization primitives
