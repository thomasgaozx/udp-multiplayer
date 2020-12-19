// UDPExperiment.cpp : Defines the entry point for the application.
//

#include "UDPExperiment.h"
#include "delta.h"
#include "object.h"

using namespace std;

constexpr float TOL = 1E-6f;

void UnitTestPacket(bool bigEndian) {
	unitTestSetEndian(bigEndian);

	uint64_t A = 0xff00ee334466aa55;
	uint64_t B = 0x123;
	int32_t C = 0x00ff3344;
	int32_t D = 0xff000000;
	double E = 0.124385738472;
	double F = 10024.1324324738742;
	float G = 13.24323f;
	float H = 0.11111243f;
	uint8_t I = 0x01;

	auto writer = PacketWriter();
	writer << A << B << C << D << E << F << G << H << I;

	uint64_t a, b;
	uint32_t c, d;
	double e, f;
	float g, h;
	uint8_t i;

	auto reader = PacketReader(writer.flush());
	reader >> a >> b >> c >> d >> e >> f >> g >> h >> i;

	assert((a == A));
	assert((b == B));
	assert((c == C));
	assert((d == D));
	assert((abs(e - E) < TOL));
	assert((abs(f - F) < TOL));
	assert((abs(g - G) < TOL));
	assert((abs(h - H) < TOL));
	assert((i == I));

	cout << "UnitTestPacket Completed!" << endl;
}

void UnitTestSerialize() {
	CreatureStatus status0{};
	CreatureStatus status1{};
	CreatureStatus status2{};

	status1.x = 5.1f;
	status1.z = 3.2f;
	status1.heading = 0.5555555555;
	status1.animframe = 3;
	status1.faction = 5;

	status2.x = 5.5f;

	auto writer = PacketWriter();
	writeDelta<CreatureStatus>(writer, status1, status0);
	auto tmp1 = writer.flush();

	writeDelta<CreatureStatus>(writer, status2, status0);
	auto tmp2 = writer.flush();

	cout << "status1 delta string size: " << tmp1.size() << endl;
	cout << "status2 delta string size: " << tmp2.size() << endl;

	assert((tmp1.size() > tmp2.size()));

	vector<char> tmp; // merge tmp1 tmp2
	tmp.reserve(tmp1.size() + tmp2.size());
	tmp.insert(tmp.end(), tmp1.begin(), tmp1.end());
	tmp.insert(tmp.end(), tmp2.begin(), tmp2.end());
	
	auto reader = PacketReader(move(tmp));
	CreatureStatus res1{}, res2{};

	// read tmp1 first
	readDelta(reader, res1);
	readDelta(reader, res2);

	cout << "res1: ";
	cout << res1.x << ", "
		<< res1.z << ", "
		<< res1.heading << ", "
		<< static_cast<uint32_t>(res1.animframe) << ", "
		<< static_cast<uint32_t>(res1.faction) << endl;
	cout << "res2: " << res2.x << endl;

	assert((abs(res1.x - status1.x) < TOL));
	assert((abs(res1.z - status1.z) < TOL));
	assert((abs(res1.heading - status1.heading) < TOL));
	assert((res1.animframe == status1.animframe));
	assert((res1.faction == status1.faction));
	assert((abs(res2.x - status2.x) < TOL));

	cout << "UnitTestSerialize Completed!" << endl;
}

int main()
{
	UnitTestPacket(true);
	UnitTestPacket(false);
	UnitTestSerialize();

	cout << "Hello CMake." << endl;
	return 0;
}
