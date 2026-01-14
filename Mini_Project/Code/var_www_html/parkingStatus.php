<?php
// 1. 타임존 설정 (한국 시간으로 고정)
date_default_timezone_set('Asia/Seoul');

// 2. DB 연결
$conn = mysqli_connect("localhost", "iot", "pwiot", "iotdb");
if (!$conn) { die("Connection failed: " . mysqli_connect_error()); }

// 3. 주차장 상태 쿼리 (정수 ID 1, 2, 3)
$sql = "SELECT id, state, time FROM parking WHERE id IN (1, 2, 3) ORDER BY id ASC";
$result = mysqli_query($conn, $sql);

$parking_data = array();
while($row = mysqli_fetch_assoc($result)) {
    $parking_data[(int)$row['id']] = $row;
}
?>

<!DOCTYPE html>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <title>AIoT 주차 관리 시스템</title>
    <style>
        :root { 
            --bg-color: #1a1a2e; 
            --card-bg: #16213e; 
            --empty: #27ae60; 
            --occupied: #e74c3c; 
        }
        body { font-family: 'Malgun Gothic', sans-serif; background-color: var(--bg-color); color: white; text-align: center; margin: 0; padding: 20px; }
        .lot-container { display: flex; justify-content: center; gap: 20px; margin-top: 50px; }
        .spot-card { 
            width: 250px; border-radius: 15px; background: var(--card-bg); 
            border-bottom: 8px solid #555; padding: 40px 20px; transition: 0.3s;
            box-shadow: 0 10px 20px rgba(0,0,0,0.3);
        }
        /* 상태별 테두리 색상 강조 */
        .occupied { border-color: var(--occupied); }
        .empty { border-color: var(--empty); }
        
        .car-img { font-size: 80px; margin: 25px 0; min-height: 100px; }
        .time-display { 
            font-size: 1em; color: #bdc3c7; 
            background: rgba(0,0,0,0.3); padding: 10px; border-radius: 5px;
            margin-top: 20px;
        }
        .status-text { margin-top: 15px; font-size: 1.4em; font-weight: bold; }
        .nav-btn { 
            display: inline-block; margin-top: 50px; padding: 15px 30px; 
            background: #0f3460; color: white; text-decoration: none; border-radius: 8px;
            font-weight: bold; transition: 0.3s;
        }
        .nav-btn:hover { background: #e94560; }
    </style>
    <script>
        // 5초마다 실시간 갱신
        setTimeout(() => location.reload(), 5000);
    </script>
</head>
<body>
    <h1 style="margin-top: 30px;">🅿️ 실시간 주차 현황판</h1>
    <p style="color: #bdc3c7;">실시간 주차 상태를 확인합니다.</p>
    
    <div class="lot-container">
        <?php for($i = 1; $i <= 3; $i++): 
            $row = isset($parking_data[$i]) ? $parking_data[$i] : ['state'=>0, 'time'=>'--:--:--'];
            $is_occupied = (int)$row['state'] === 1;
        ?>
        <div class="spot-card <?php echo $is_occupied ? 'occupied' : 'empty'; ?>">
            <div style="font-size: 1.5em; font-weight: bold; color: #95a5a6;">SPOT 0<?php echo $i; ?></div>
            
            <div class="car-img"><?php echo $is_occupied ? "🚗" : "🅿️"; ?></div>
            
            <div class="status-text" style="color: <?php echo $is_occupied ? 'var(--occupied)':'var(--empty)'; ?>">
                <?php echo $is_occupied ? "주차 중 (입차)" : "공석 (출차)"; ?>
            </div>

            <div class="time-display">
                <?php echo $is_occupied ? "입차시간: " . $row['time'] : "상태변경: " . $row['time']; ?>
            </div>
        </div>
        <?php endfor; ?>
    </div>

    <a href="parkingLog.php" class="nav-btn">이용 로그 전체보기</a>

</body>
</html>
<?php mysqli_close($conn); ?>