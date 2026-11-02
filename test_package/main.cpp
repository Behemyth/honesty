import synodic.honesty;
import synodic.periapsis;

int main()
{
	try
	{
		throw peri::InvalidStateException {};
	}
	catch (const peri::InvalidStateException&)
	{
		return 0;
	}

	return 0;
}
