<?php
// 1. DB 연결
$conn = mysqli_connect("localhost", "iot", "pwiot", "iotdb");

if (!$conn) {
    die("Connection failed: " . mysqli_connect_error());
}

// 2. 통계 데이터 쿼리 (ID별 입차(IN) 횟수 계산)
$stats_sql = "SELECT id, COUNT(*) as cnt FROM log WHERE state='IN' GROUP BY id";
$stats_result = mysqli_query($conn, $stats_sql);
$chart_data = array();
while($row = mysqli_fetch_assoc($stats_result)) {
    $chart_data[] = "['Spot " . $row['id'] . "', " . $row['cnt'] . "]";
}
$chart_js_data = implode(",", $chart_data);

// 3. 최근 로그 리스트 쿼리 (최신순 20개)
$log_sql = "SELECT id, date, time, state FROM log ORDER BY date DESC, time DESC LIMIT 20";
$log_result = mysqli_query($conn, $log_sql);
?>

<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>주차장 이용 로그</title>
    <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
    <script type="text/javascript">
        google.charts.load('current', {'packages':['corechart']});
        google.charts.setOnLoadCallback(drawChart);

        function drawChart() {
            var data = google.visualization.arrayToDataTable([
                ['주차칸', '총 이용 횟수'],
                <?php echo $chart_js_data; ?>
            ]);

            var options = {
                title: '주차칸별 누적 이용 횟수 (IN 기준)',
                hAxis: {title: '주차칸'},
                vAxis: {minValue: 0},
                colors: ['#3498db']
            };

            var chart = new google.visualization.ColumnChart(document.getElementById('stats_chart'));
            chart.draw(data, options);
        }
    </script>
    <style>
        body { font-family: 'Malgun Gothic', sans-serif; padding: 20px; background-color: #f4f7f6; }
        .container { max-width: 900px; margin: auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        table { width: 100%; border-collapse: collapse; margin-top: 20px; }
        th, td { padding: 12px; border-bottom: 1px solid #ddd; text-align: center; }
        th { background-color: #2c3e50; color: white; }
        tr:hover { background-color: #f1f1f1; }
        .state-in { color: #2ecc71; font-weight: bold; }
        .state-out { color: #e74c3c; font-weight: bold; }
        .nav-btn { display: inline-block; padding: 10px 20px; background: #34495e; color: white; text-decoration: none; border-radius: 5px; margin-bottom: 20px; }
    </style>
</head>
<body>

<div class="container">
    <a href="parkingStatus.php" class="nav-btn">← 실시간 현황판 보기</a>
    <h2 style="text-align: center;">📜 주차장 이용 로그 레코드</h2>

    <div id="stats_chart" style="width: 100%; height: 300px;"></div>

    <table>
        <thead>
            <tr>
                <th>ID (주차칸)</th>
                <th>날짜</th>
                <th>시간</th>
                <th>상태 (IN/OUT)</th>
            </tr>
        </thead>
        <tbody>
            <?php while($row = mysqli_fetch_assoc($log_result)): 
                $state_class = ($row['state'] == 'IN') ? 'state-in' : 'state-out';
            ?>
            <tr>
                <td><b><?php echo $row['id']; ?></b></td>
                <td><?php echo $row['date']; ?></td>
                <td><?php echo $row['time']; ?></td>
                <td class="<?php echo $state_class; ?>"><?php echo $row['state']; ?></td>
            </tr>
            <?php endwhile; ?>
        </tbody>
    </table>
</div>

</body>
</html>
<?php mysqli_close($conn); ?>
