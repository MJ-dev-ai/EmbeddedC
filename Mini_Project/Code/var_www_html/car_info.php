<?php
header("Content-Type: application/json");
header("Access-Control-Allow-Origin: *");

/* ================= DB 연결 ================= */
$conn = new mysqli("localhost", "iot", "pwiot", "iotdb");
if ($conn->connect_error) {
    echo json_encode(["error" => "DB connection failed"]);
    exit;
}

$prefix = $_GET["prefix"] ?? "";

/* ================= 차량 정보 ================= */
$car = $conn->query("
    SELECT id, state
    FROM car
    WHERE id LIKE '{$prefix}%'
    LIMIT 1
")->fetch_assoc();

if (!$car) {
    echo json_encode(["current" => null, "logs" => []]);
    exit;
}

/* ================= 마지막 ENTER 조회 ================= */
$lastEnter = $conn->query("
    SELECT date, time
    FROM car_log
    WHERE id = '{$car['id']}'
      AND state = 'ENTER'
    ORDER BY date DESC, time DESC
    LIMIT 1
")->fetch_assoc();

$enterTimeStr = null;
$currentFee = 0;

/* ================= 현재 요금 (DB 시간 기준) ================= */
if ((int)$car["state"] === 1 && $lastEnter) {

    $enterTimeStr = $lastEnter["date"] . " " . $lastEnter["time"];

    $feeRow = $conn->query("
        SELECT
            CASE
                WHEN TIMESTAMPDIFF(SECOND,
                    '{$enterTimeStr}',
                    NOW()
                ) >= 86400 THEN 40000
                ELSE LEAST(
                    FLOOR(
                        GREATEST(
                            TIMESTAMPDIFF(SECOND,
                                '{$enterTimeStr}',
                                NOW()
                            ),
                            0
                        ) / 60
                    ) * 1000,
                    40000
                )
            END AS fee
    ")->fetch_assoc();

    $currentFee = (int)$feeRow["fee"];
}

/* ================= 로그 (전부 출력, LEAVE만 요금) ================= */
$logs = [];
$lastEnterTime = null;

$res = $conn->query("
    SELECT date, time, state
    FROM car_log
    WHERE id = '{$car['id']}'
    ORDER BY date, time
");

while ($row = $res->fetch_assoc()) {

    $fee = null;

    if ($row["state"] === "ENTER") {
        $lastEnterTime = $row["date"] . " " . $row["time"];
    }
    else if ($row["state"] === "LEAVE") {

        if ($lastEnterTime) {

            $feeRow = $conn->query("
                SELECT
                    CASE
                        WHEN TIMESTAMPDIFF(SECOND,
                            '{$lastEnterTime}',
                            '{$row['date']} {$row['time']}'
                        ) >= 86400 THEN 40000
                        ELSE LEAST(
                            FLOOR(
                                GREATEST(
                                    TIMESTAMPDIFF(SECOND,
                                        '{$lastEnterTime}',
                                        '{$row['date']} {$row['time']}'
                                    ),
                                    0
                                ) / 60
                            ) * 1000,
                            40000
                        )
                    END AS fee
            ")->fetch_assoc();

            $fee = (int)$feeRow["fee"];
            $lastEnterTime = null;
        } else {
            $fee = 0;
        }
    }

    $logs[] = [
        "date"  => $row["date"],
        "time"  => $row["time"],
        "state" => $row["state"],
        "fee"   => $fee
    ];
}

/* ================= JSON 출력 ================= */
echo json_encode([
    "current" => [
        "id"         => $car["id"],
        "state"      => (int)$car["state"],
        "enter_time" => $enterTimeStr,   // ← 입차 시간 표시 핵심
        "fee"        => $currentFee
    ],
    "logs" => $logs
]);
